/*
The MIT License (MIT)

Copyright (c) 2016 Matthew Jones

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "ServerLink.h"
#include "FriendLink-Common\NetworkExceptions.h"
#include "FriendLink-Common\Error.h"

using namespace std;
namespace Network {
ServerLink::ServerLink(std::string ip, std::string port, 
                       std::string udp_port, HandleReceived& r)
	  : handle_received_(r) {
	//Setup TCP socket
	AddressHint hints{AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP};
	Address address_tcp{ ip, port, hints };
  socket_tcp_ = move(ConnectTo(address_tcp));

	//Setup receive udp socket
	AddressHint hints_receive {AF_INET,SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE};
	Address address_receive{ {}, port, hints_receive };
  socket_receive_ = move(BindTo(address_receive));

	//Setup send udp socket
	AddressHint hints_send{PF_INET, SOCK_DGRAM, IPPROTO_UDP};
	Address address_send{ ip, udp_port, hints_send };
  socket_send_ = move(ConnectTo(address_send));
}
ServerLink::~ServerLink() {
  Error::LogToFile("ServerLink deleted Start");
	Disconnect();
  thread_send_.join();
  Close(socket_receive_.get());
  thread_receive_udp_.join();
  thread_receive_tcp_.join();
  Error::LogToFile("ServerLink deleted End");
}
void ServerLink::SendReliable(Game::Property::Stream properties) {
	Packet::Packet packet 
      { Packet::Type::kProperties, uint8_t(server_slot_), properties.Packed() };
  SendReliable(packet);
}
void ServerLink::SendReliable(Packet::Packet packet) {
	lock_guard<mutex> lck(reliable_data_write_mtx_);
  reliable_data_.Push(packet.Packed());
}
void ServerLink::Send(Game::Property::Stream properties) {
	Packet::Packet packet
      { Packet::Type::kProperties, uint8_t(server_slot_), properties.Packed() };
  data_.Push(packet.Packed());
}
void ServerLink::SendDataRequest() {
	Packet::Packet packet 
      { Packet::Type::kDataRequest, uint8_t(server_slot_), {'0'} };
  SendReliable(packet);
}
void ServerLink::StartDataTransfer() {
  thread_send_ = thread(&ServerLink::SendLoop, this);
  thread_receive_tcp_ = thread(&ServerLink::ReceiveLoop_tcp, this);
  thread_receive_udp_ = thread(&ServerLink::ReceiveLoop, this);
}
void ServerLink::Disconnect() {
	connected_ = false;
}
bool ServerLink::IsActive() const {
	return connected_ && IsValid();
}
void ServerLink::ReceiveInitialMessage() {
  while (IsActive()) {
    Error::LogToFile("0 ReceiveInitialmessage");
    auto packet = Packet::Receive(*socket_tcp_);
    Error::LogToFile("1 ReceiveInitialmessage");
    if (packet.type() == Packet::Type::kInitialMessage) {
      Error::LogToFile("2 ReceiveInitialmessage");
      auto init_message = InitialMessage(packet);
      Error::LogToFile("3 ReceiveInitialmessage");
      handle_received_.InitialMessageData(init_message);
      server_slot_ = init_message.client_index();
      if (server_slot_ >= init_message.max_clients()) {
        handle_received_.ErrorMessage("Server is full.");
        Error::LogToFile("Server is full.");
        Disconnect();
      }
      return;
    } else if (packet.type() == Packet::Type::kSocketDisconnect) {
      Disconnect();
      return;
    }
  }
  Error::LogToFile("initial message recieved.");
}
void ServerLink::SendLoop() {
	while (IsActive()) {
		try {
			auto reliable_packet = reliable_data_.Pop();
			if (reliable_data_) {
				Network::Send(*socket_tcp_, reliable_packet);
			}

      auto packet = data_.Pop();
      if(data_) {
        Network::Send(*socket_send_, packet);
      }

      if(!reliable_data_ && !data_) { //both had nothing to send then sleep
        this_thread::sleep_for(chrono::milliseconds(1));
      }
		} catch (NetworkException e) {
			Error::LogToFile(e.what(), e.code(), e.message());
      handle_received_.ErrorMessage(e.what());
			Disconnect();
			break;
		}
	}

  //Close(socket_send_.get());
	Shutdown(socket_tcp_.get(), SD_SEND);
}
void ServerLink::ReceiveLoop() {
	while (IsValid() && socket_receive_ && socket_receive_->IsValid()) {
		try {
			auto packet = Packet::Receive(*socket_receive_);
			if (packet.type() == Packet::Type::kSocketDisconnect) {
        break;
      } else {
        handle_received_.PacketData(packet);
      }
		} catch (RecvException e) {
			if (e.code() != WSAEINTR) { //When closed while trying to receive.
				Error::LogToFile(e.what(), e.code(), e.message());
        handle_received_.ErrorMessage(e.what());
			}
			break;
		} catch (NetworkException e) {
			Error::LogToFile(e.what(), e.code(), e.message());
      handle_received_.ErrorMessage(e.what());
			break;
		}
	}
  Disconnect();
}
void ServerLink::ReceiveLoop_tcp() {
  while (IsValid()) {
    try {
      auto packet = Packet::Receive(*socket_tcp_);
      if (packet.type() == Packet::Type::kSocketDisconnect) {
        break;
      } else {
        handle_received_.PacketData(packet);
      }
    } catch (NetworkException e) {
      Error::LogToFile(e.what(), e.code(), e.message());
      handle_received_.ErrorMessage(e.what());
      break;
    }
  }
  Disconnect();
  handle_received_.Disconnection();
}
bool ServerLink::IsValid() const {
  return socket_tcp_ && socket_tcp_->IsValid();
}
void ServerLink::Close(Socket* sckt) {
  try {
    sckt->Close();
  } catch (CloseSocketException e) {
    Error::LogToFile(e.what(), e.code(), e.message());
    handle_received_.ErrorMessage(e.what());
  }
}
void ServerLink::Shutdown(Socket* sckt, int how) {
  try {
    sckt->Shutdown(how);
  } catch (ShutdownException e) {
    Error::LogToFile(e.what(), e.code(), e.message());
    handle_received_.ErrorMessage(e.what());
  }
}
}