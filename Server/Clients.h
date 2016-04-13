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
#pragma once
#include "stdafx.h"
#include <atomic>
#include <thread>
#include <future>
#include "FriendLink-Common/game_structures.h"
#include "FriendLink-Common/DataHandling.h"
#include "FriendLink-Common/Sharing.h"

namespace Network {
class Clients;
/**
 *  Handles the connection of an individual client.
 */
class Client {
	typedef std::vector<std::shared_ptr<Data::Sharing::FixedQueue>> 
      MultiWriteShareQueue;
 public:
  /**
   *  @param socket_tcp A connected socket with type tcp.
   *  @param socket_udp A connected socket with type udp.
   */
	Client(Clients& clients, size_t size, size_t my_server_slot, 
          std::unique_ptr<Socket> socket_tcp, 
          std::unique_ptr<Socket> socket_udp);
	~Client();
  /**
   *  Is the client connected?
   */
	bool IsActive();
  /**
    *  Handles disconnecting the client.
    */
	void Disconnect();
  /**
   *  Sends a packet to the client and make sure that the client recieves it.
   *  @remark This currently sends a packet through tcp. However, this will
   *          once the transition to udp is complete.
   */
  void SendReliable(Packet::Packet);
  /**
   *  Sends a packet to the client without checking to see if the packet was 
   *  received.
   */
	void QueueToSend(Packet::Packet);
  /**
   *  The server slot that this client is in. Used as an unique i.d. for the 
   *  client.
   */
	size_t my_server_slot() { return my_server_slot_; }
		
 private:
  void SendLoop();
  void ReceiveLoop();
	std::atomic<bool> connected = false;
	std::unique_ptr<Socket> socket_tcp_;
	std::unique_ptr<Socket> socket_udp_;
	std::thread thread_receive_;
	std::thread thread_send_;
	size_t my_server_slot_;
  MultiWriteShareQueue reliable_data_;
  MultiWriteShareQueue data_;
  Clients& clients_;
};
/**
 *  Keeps track of all the connected clients and allows you to send data to all
 *  of them.
 */
class Clients {
public:
  /**
   *  @param size the max number of clients the server will accept.
   */
	Clients(size_t size) : clients(size) {}
  /**
   *  Sends the packet to all clients except for the client where the packet
   *  came from.
   *  @todo Add a id to the server so the server can send a message to all
   *        clients.
   */
	void SendReliableToAll(Packet::Packet);
  /**
   *  Sends the packet to all clients except for the client where the packet
   *  came from.
   *  @todo Add a id to the server so the server can send a message to all
   *        clients.
   */
	void SendToAll(Packet::Packet);
  /**
   *  Adds a new connected client to the server. 
   *  @param socket_tcp A connected socket with type tcp.
   *  @param socket_udp A connected socket with type udp.
   *  @return True if successfully added.
   */
	bool Push(std::unique_ptr<Socket> socket_tcp, 
            std::unique_ptr<Socket> socket_udp);
  /**
  *  Sends information about the rest of the connected clients to socket.
  *  @param socket_tcp A connected socket with type tcp.
  */
	void SendInitialClientDataTo(Socket& socket);
private:
	std::vector<std::unique_ptr<Client>> clients;
};
/**
 *  Handles the two listening sockets. Setups up new connections and relays 
 *  data to the clients when received data comes in. 
 *  When constructed the server is online and becomes offline when this object
 *  is destroyed.
 */
class Listen {
 public:
   /**
   *  @param max_clients max number of clients allowed on the server.
   *  @param port tcp port that the server is using.
   *  @param port_udp udp port that the server is using.
   */
  Listen(size_t max_clients, std::string port, std::string port_udp);
  Listen(Listen&) = delete;
  Listen(Listen&&) = delete;
  ~Listen();

 private:
  void Tcp();
  void AcceptClient();
  void Udp();
  
  Wsa wsa_startup_ {};
  Clients clients_;
  u_short client_port_ = (u_short)strtoul(kDefaultPortClientReceiver, NULL, 0);
  u_short server_port_ = (u_short)strtoul(kDefaultPortServerReceiver, NULL, 0);
  std::unique_ptr<Socket> listener_tcp_;
  std::unique_ptr<Socket> listener_udp_;
  std::thread thread_tcp_;
  std::thread thread_udp_;
};
}