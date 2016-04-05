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
#include "Clients.h"
#include <assert.h>
#include "ConsoleDisplay/Console.h"
#include "FriendLink-Common/NetworkExceptions.h"
#include "FriendLink-Common/Error.h"

using namespace std;
namespace Network {
Client::Client(Clients& clients, size_t size, size_t my_server_slot,
               unique_ptr<Socket> socket_tcp,
               unique_ptr<Socket> socket_udp)
	  : clients_(clients),
      my_server_slot_(my_server_slot), 
      socket_tcp_(move(socket_tcp)),
      socket_udp_(move(socket_udp)),
      connected { true } {
  //Setup the sending queues.
  reliable_data_.resize(size);
  data_.resize(size);
	//load the dataStream with unique shared ptrs at each index.
	for (size_t i = 0; i < data_.size(); ++i) {
    data_[i] = make_shared<Data::Sharing::FixedQueue>();
    reliable_data_[i] = make_shared<Data::Sharing::FixedQueue>();
	}

	assert(size < 256 && index < 256);
	Send(*socket_tcp_, InitialMessage{ uint8_t(size), uint8_t(my_server_slot_) });
  clients_.SendReliableToAll(
      Packet::PackStatus(uint8_t(my_server_slot_), Packet::Status::kNew));
  clients.SendInitialClientDataTo(*socket_tcp_);
	clients.SendReliableToAll(Packet::RequestData(uint8_t(my_server_slot_)));

  thread_receive_ = thread(&Client::ReceiveLoop, this);
  thread_send_ = thread(&Client::SendLoop, this);
}
Client::~Client() {
	Disconnect();
  thread_send_.join();
  thread_receive_.join();
}
bool Client::IsActive() { 
	return connected && 
         socket_tcp_ && socket_tcp_->IsValid() &&
         socket_udp_ && socket_udp_->IsValid();
}
void Client::Disconnect() { 
	connected = false;
  clients_.SendReliableToAll(
      Packet::PackStatus(uint8_t(my_server_slot_), 
                         Packet::Status::kDisconnected));
}
void Client::SendReliable(Packet::Packet packet) {
  if (IsActive() && my_server_slot_ != packet.client()) {
    reliable_data_[packet.client()]->Push(packet.Packed());
  }
}
void Client::QueueToSend(Packet::Packet packet) {
  if (IsActive() && my_server_slot_ != packet.client()) {
    data_[packet.client()]->Push(packet.Packed());
	}
}
void Client::SendLoop() {
	while (connected) {
		try {
      bool something_sent = false;
			for (size_t i = 0; i < data_.size(); ++i) {
				auto reliable_packet = reliable_data_[i]->Pop();
				if (*reliable_data_[i]) {
					Packet::Send(*socket_tcp_, reliable_packet);
          something_sent = true;
				}
        auto packet = data_[i]->Pop();
        if (*data_[i]) {
          Packet::Send(*socket_udp_, packet);
          something_sent = true;
        }
			}
      if(something_sent) {
        this_thread::sleep_for(chrono::milliseconds(1));
      }
		} catch (NetworkException e) {
			Error::LogToFile(e.what(), e.code(), e.message());
			Disconnect();
		}
	}

  try {
    //Network::Send(*socket_udp_, {});
    socket_udp_->Shutdown(SD_SEND);
    socket_tcp_->Shutdown(SD_SEND);
  } catch (ShutdownException e) {
    Error::LogToFile(e.what(), e.code(), e.message());
  }
}
void Client::ReceiveLoop() {
	while (IsActive()) {
		try{
			auto packet = Packet::Receive(*socket_tcp_);
			switch (packet.type()) {
      case Packet::Type::kDataRequest:
			case Packet::Type::kProperties:
				clients_.SendReliableToAll(packet);
				break;
			case Packet::Type::kSocketDisconnect:
				Disconnect();
				return;
			}
		} catch (NetworkException e) {
			Error::LogToFile(e.what(), e.code(), e.message());
      Disconnect(); 
      return;
		}
	}
}
void Clients::SendReliableToAll(Packet::Packet packet) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (i != packet.client() && clients[i]) {
			clients[i]->SendReliable(packet);
		}
	}
  Display::Console::AddReliableReceivedData(packet);
}
void Clients::SendToAll(Network::Packet::Packet packet) {
	if (clients[packet.client()] && clients[packet.client()]->IsActive()) {
		for (size_t i = 0; i < clients.size(); ++i) {
			if (clients[i] && clients[i]->IsActive()) {
				clients[i]->QueueToSend(packet);
			}
		}
    Display::Console::AddReceivedData(packet);
	}
}
bool Clients::Push(unique_ptr<Socket> socket_tcp, 
                   unique_ptr<Socket> socket_udp) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (!clients[i] || !clients[i]->IsActive()) {
			clients[i].reset(
        new Client(*this, clients.size(), i, 
                   move(socket_tcp), 
                   move(socket_udp)));
			return true;
		}
	}
  return false;
}
void Clients::SendInitialClientDataTo(Socket& socket) {
	for (size_t i = 0; i < clients.size(); ++i) {
		if (clients[i] && clients[i]->IsActive()) {
			Send(socket, Packet::PackStatus(uint8_t(i), Packet::Status::kNew));
		}
	}
}
Listen::Listen(size_t maxClients, std::string port, std::string port_udp) 
    : clients_(maxClients) {
  client_port_ = (u_short)strtoul(port_udp.c_str(), NULL, 0);
  //Bind udp listener
  AddressHint hints_udp {AF_INET,SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE};
  Address address_udp{{}, port_udp, hints_udp};
  listener_udp_ = move(BindTo(address_udp));

  //Bind tcp listener
  AddressHint hints_tcp{AF_INET, SOCK_STREAM, IPPROTO_TCP, AI_PASSIVE};
  Address address_tcp{{}, port, hints_tcp};
  listener_tcp_ = move(BindTo(address_tcp));

  thread_tcp_ = thread{&Listen::Tcp, this};
  thread_udp_ = thread{&Listen::Udp, this};
}
Listen::~Listen() {
  //deleting the socket ends the loop.
  listener_tcp_.reset();
  listener_udp_.reset();  
  thread_tcp_.join();
  thread_udp_.join();
}
void Listen::Tcp() {
  try {
    ListenOn(*listener_tcp_);
    while (listener_tcp_ && listener_tcp_->IsValid()) {
      AcceptClient();
    }
  } catch (NetworkException e) {
    Display::Console::PrintError(e.what());
    Error::LogToFile(e.what(), e.code(), e.message());
  }
}
void Listen::AcceptClient() {
  try {
    auto address = make_shared<sockaddr_in>();
    auto client_socket_tcp = Accept(*listener_tcp_, address);

    //Connecting to client receiver
    SetAddressPort(*address, client_port_);
    auto client_socket_udp = ConnectTo(*address, SOCK_DGRAM, IPPROTO_UDP);

    if (clients_.Push(move(client_socket_tcp), move(client_socket_udp))) {
      Error::LogToFile("Connected: "+AddressToString(*address));
    } else {
      Send(*client_socket_tcp, InitialMessage{0, 0}); //Server doesn't accept
    }
  } catch (AcceptException e) {
    if (e.code() != WSAEINTR) {//Happens when I close the  listening socket to force the accept to return.
      Display::Console::PrintError(e.what());
      Error::LogToFile(e.what(), e.code(), e.message());
    }
  }
}
void Listen::Udp() {
  try {
    while (listener_udp_ && listener_udp_->IsValid()) {
      auto packet = Packet::Receive(*listener_udp_);
      switch (packet.type()) {
        case Packet::Type::kProperties:
          clients_.SendToAll(packet);
          break;
        default:
          //this_thread::sleep_for(chrono::milliseconds(Connection::latency));
          break;
      }
    }
  } catch (RecvException e) {
    if (e.code() != WSAEINTR && e.code() != WSAENOTSOCK) {
      Error::LogToFile(e.what(), e.code(), e.message());
      Display::Console::PrintError(e.what());
    }
  } catch (NetworkException& err) {
    Display::Console::PrintError(err.what());
    Error::LogToFile(err.what(), err.code(), err.message());
  }
}
}//namespace Network