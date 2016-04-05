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
/** @file
 *  Contains the tools to connect and use the information from the server.
 */
#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include "FriendLink-Common\Connection.h"
#include "FriendLink-Common\DataHandling.h"
#include "FriendLink-Common\Sharing.h"
#include "FriendLink-Common\game_structures.h"

namespace Network {
/**
 *  This class gives the ability to handle the important data from ServerLink.
 *  @see ServerLink
 */
class HandleReceived {
 public:
  /**
   *  This is called when recieving the inital message from the server.
   */
	virtual void InitialMessageData(InitialMessage) = 0;
  /**
   *  This is called when the server initiates a disconnection with this client.
   */
	virtual void Disconnection() = 0;
  /**
   *  This is called when receiving a packet.
   *  @remark Currently called by two threads.
   */
  virtual void PacketData(Packet::Packet) = 0;
  /**
   *  All errors are reported to the log but summary is also sent through this
   *  method.
   *  @remark Requires to be thread safe.
   */
	virtual void ErrorMessage(std::string) = 0;
 protected:
  HandleReceived() {}
  std::mutex mtx;
};
/**
 *  The main link to the server. Use a derivative of HandleReceived to use the 
 *  the information received by ServerLink. 
 */
class ServerLink {
 public:
  /**
   *  Connects to the server.
   *  @param ip Server ip address.
   *  @param port tcp port.
   *  @param udp_port udp port.
   *  @exception @see ConnectTo @see BindTo @see AddrHint @see Address
   */
	ServerLink(std::string ip, 
             std::string port, 
             std::string udp_port, 
             HandleReceived&);
	ServerLink(ServerLink&) = delete;
	ServerLink(ServerLink&&) = delete;
	~ServerLink();
  /**
   *  Send a packet and marks it as reliable to make sure the server receives
   *  it.
   *  @param properties A stream of properties that will packed into a packet.
   *  @remark This is thread safe.
   */
	void SendReliable(Game::Property::Stream properties);
  /**
   *  Send a packet and marks it as reliable to make sure the server receives
   *  it.
   *  @remark This is thread safe.
   */
	void SendReliable(Packet::Packet);
  /**
   *  Send a stream of properties but doesn't check to see if they are received.
   *  @remark This is not thread safe.
   */
	void Send(Game::Property::Stream);
  /**
   *  Sends a data request to the server. It will ask all the clients for an
   *  update.
   */
	void SendDataRequest();
  /**
   *  Starts sending and receiving of data to and from the server.
   */
	void StartDataTransfer();
  /**
   *  Disconnect from the server. 
   *  @remark This is not necessary to call as destorying
   *          this object will also disconnect the server and should be the
   *          prefered method.
   */
	void Disconnect();
  /**
   *  Checks to see if the connection is still active.
   *  @return True if the server is connected.
   */
	bool IsActive() const;
  /**
   *  Receives the initial message from the server.
   */
	void ReceiveInitialMessage();

 private:
	std::unique_ptr<Socket> socket_tcp_;
	std::unique_ptr<Socket> socket_send_;
	std::unique_ptr<Socket> socket_receive_;
	std::thread thread_receive_tcp_;
	std::thread thread_receive_udp_;
	std::thread thread_send_;
	std::mutex reliable_data_write_mtx_;
	size_t server_slot_ = 0;
	std::atomic<bool> connected_ = true;
	Data::Sharing::FixedQueue reliable_data_; //Better data structure required.
  Data::Sharing::FixedQueue data_;
  HandleReceived& handle_received_;

	void SendLoop();
  void ReceiveLoop();
	void ReceiveLoop_tcp(); //Temporary until the complete switch to udp is made.
	bool IsValid() const;
  /**
   *  Safe way to close a socket. Logs an error to the handle_received as well
   *  as the log file. No exceptions are thrown.
   */
  void Close(Socket*);
  /**
   *  Safe way to shutdown a socket. Logs an error to the handle_received as
   *  well as the log file. No exceptions are thrown.
   */
  void Shutdown(Socket*, int how);
};
}