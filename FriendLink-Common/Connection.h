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
 *  Directly access winsock and wrap it in a resource safe api.  
 */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <WinSock2.h>
#include "Buffers.h"

namespace Network {
static constexpr auto kDefaultPortClientReceiver = "29015";
static constexpr auto kDefaultPortServerReceiver = "29016";
static constexpr int kBufferSize = 1024;
static constexpr int kAntiCongestion = 35; //milliseconds

/**
 *  A wrapper for WSAStartup (using version 2,2) and WSACleanup.
 */
class Wsa {
 public:
	/** 
   *  @exception Throws exception: WSADataException
   */
	Wsa();
	Wsa(Wsa&) = delete;
	Wsa(Wsa&&) = delete;
	//Logs any cleanup error to log file.
	~Wsa();

 private:
	WSADATA wsa_data_;
	void LogCleanupError(int err);
};

/**
 *  A wrapper for address hints. Easier construction and insure that memory is
 *  zeroed. The parameters are used the same as the winsock addrinfo structure.
 *  @link https://msdn.microsoft.com/en-ca/library/windows/desktop/ms737530(v=vs.85).aspx
 */
class AddressHint {
 public:
  /**
   *  Empty hint address that is zeroed.
   */
  AddressHint();
  /**
   *  @param ai_family Same as ai_family from addrinfo.
   *  @param ai_socktype Same as ai_socktype from addrinfo.
   *  @param ai_protocol Same as ai_protocol from addrinfo.
   *  @param ai_flags Same as ai_flags from addrinfo.
   */
  AddressHint(int ai_family, int ai_socktype, int ai_protocol, int ai_flags);
  /**
   *  @param ai_family Same as ai_family from addrinfo.
   *  @param ai_socktype Same as ai_socktype from addrinfo.
   *  @param ai_protocol Same as ai_protocol from addrinfo.
   */
  AddressHint(int ai_family, int ai_socktype, int ai_protocol);
  AddressHint(AddressHint&) = delete;
  AddressHint(AddressHint&&) = delete;
  
  /**
   *  @param ai_flags Same as ai_flags from addrinfo.
   */
	void SetFlags(int ai_flags);
  /**
   *  @param ai_family Same as ai_family from addrinfo.
   */
	void SetFamily(int ai_family);
  /**
   *  @param ai_socktype Same as ai_socktype from addrinfo.
   */
	void SetSockType(int ai_socktype);
  /**
   *  @param ai_protocol Same as ai_protocol from addrinfo.
   */
	void SetProtocol(int ai_protocol);
  const addrinfo& get() const { return hints_; }
 
 private:
	addrinfo hints_;
};
/**
 *  Encloses winsock addrinfo.
 */
class Address {
 public:
  /**
   *  Creates an address list using winsocks getaddrinfo.
   *  @param host Host to find the information about. The empty string 
   *              corresponds to passing the host as NULL in getaddrinfo.
   *  @exception AddressExeption when getaddrinfo returns an error.
   */
	Address(std::string host, std::string port, const AddressHint& hints);
	Address(Address&) = delete;
	Address(Address&&) = delete;
	~Address();

  /**
   *  The current addrinfo being pointed at in the list of addrinfo.
   */
	const addrinfo& current() const { return *ptr_; }
  /**
   *  Gets the next element in the addrinfo list.
   *  @return next addrinfo or NULL if the current ptr is the end.
   */
	void Next();
  /**
   *  Checks if the list is at the end. If we are at the end resets the list.
   *  @return True when the list is not at the end.
   */
	bool NotEnd();

 private:
	addrinfo* address_ = NULL;
	addrinfo* ptr_ = NULL;
};
/**
 *  A wrapper for the SOCKET class in winsock. 
 */
class Socket {
  typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;

 public:
	Socket() {}
  /**
   *  @param ai_family Same as ai_family from addrinfo.
   *  @param ai_socktype Same as ai_socktype from addrinfo.
   *  @param ai_protocol Same as ai_protocol from addrinfo.
   *  @exception SocketException
   */
	Socket(int ai_family, int ai_socktype, int ai_protocol);
  /**
   *  @param address That the socket will be connecting to.
   *  @exception SocketException
   */
	Socket(const addrinfo& address);
  explicit Socket(SOCKET socket) : socket_(socket) {}
	Socket(Socket&) = delete;
	Socket(Socket&&) = default;
	~Socket();

	const SOCKET& Get() const { return socket_; }
  /**
   *  Data received by this socket is stored into this buffer.
   */
  Data::Buffer::Circular& recieved_data() { return recieved_data_; }
  bool IsValid() const { return socket_ != INVALID_SOCKET; }
  /**
   *  @param how Same as how param in winsock shutdown.
   *  @exception ShutdownException
   */
	void Shutdown(int how);
  /**
   *  @exception CloseSocketException
   */
	void Close();
  /**
   *  @exception SocketOptionException
   */
	bool IsTcp() const;
  /**
   *  Checks to see if data is ready to send and then updates the time of last
   *  time this socket sent data.
   *  @return True if this socket is ready to send data.
   */
  bool ReadyToSend();
 
 private:
	SOCKET socket_ = INVALID_SOCKET;
  Data::Buffer::Circular recieved_data_ {kBufferSize * 2};
  TimePoint time_last_sent_ = std::chrono::steady_clock::now();
};

/**
 *  Creates a socket bound to address.
 *  @param address Address that the new socket will use to bind.
 *  @return A socket bound to the address.
 *  @exception BindException Logs only the error received while trying to bind
 *                           the last address.
 */
std::unique_ptr<Socket> BindTo(Address& address);
/**
 *  Binds a socket to address.
 *  @param address Address that the new socket will use to bind.
 *  @param binding_socket[in/out] The socket that will be bound too.
 *  @exception BindException Logs only the error received while trying to bind
 *                           the last address.
 */
void BindTo(Address& address, Socket& binding_socket);
/**
 *  Try to bind the socket to the first address in address.
 *  @param address Only the first address in addrinfo will be bound too.
 *  @param binding_socket[in/out] The socket that will be bound too.
 *  @return True if bind was successful.
 *  @remark Use the BindTo functions.
 */
bool Bind(const addrinfo& address, Socket& binding_socket);
/**
 *  Creates a socket and connects it to address.
 *  @param address Address that the new socket will be connected to.
 *  @return A new socket conencted to the address.
 *  @exception ConnectException Logs only the error received while trying to
 *                              connect to the last address.
 */
std::unique_ptr<Socket> ConnectTo(Address& address);
/**
 *  Creates a socket and connects it to address.
 *  @param address Address that the new socket will be connected to.
 *  @param connecting_socket[in/out] Socket that will connect to address.
 *  @exception ConnectException Logs only the error received while trying to
 *                              connect to the last address.
 */
void ConnectTo(Address& address, Socket& connecting_socket);
/**
*  Creates a socket and connects it to address.
*  @return A new socket conencted to the address.
*  @exception ConnectException
*/
std::unique_ptr<Socket> ConnectTo(const sockaddr_in&, 
                                  int ai_socktype, 
                                  int ai_protocol);
/**
 *  Try to connect the socket to the first address in address.
 *  @param address Only the first address in addrinfo will be used.
 *  @param socket[in/out] Socket that will connect to first address.
 *  @return True if connection was successful.
 *  @remark Use the ConnectTo functions.
 */
bool Connect(const addrinfo& address, Socket& socket);
/**
 *  Sends all the data from vector<char> to the socket.
 *  @exception SendException
 *  @remark For udp sockets will only send data after kAntiCongestion time has
 *          elapsed. If the time hasn't elapsed then the data will be lost.
 */
void Send(Socket&, std::vector<char>);
/**
 *  Receives all n characters. 
 *  @param blocking_socket This socket should have blocking enabled or an
 *         exception will be thrown.
 *  @param n The number of characters to collect.
 *  @return vector<char> of length n or 0. When a vector of size 0 is returned
 *          the client has sent a disconnect message. Otherwise the vector will
 *          be filled with the data from the receive call.
 *  @exception RecvException
 */
std::vector<char> Receive(Socket& blocking_socket, uint32_t n);
/**
 *  Goes through received data until code is reached. The next data received
 *  will be the data after the code.
 *  @param blocking_socket This socket should have blocking enabled or an
 *         exception will be thrown.
 *  @param code Go through received data untill code is reached.
 *  @return True if received, false if the connection was disconnected.
 *  @exception RecvException
 */
bool ReceiveUntil(Socket& blocking_socket, std::vector<char> code);
/**
 *  Start listening on the socket.
 *  @exception ListenException
 */
void ListenOn(const Socket&);
/**
 *  Accepts a connection and creates a new socket.
 *  @param listen_socket A socket that is currently listening for new 
 *                       connections
 *  @return A new socket that a connection has been accepted.
 *  @exception AcceptException
 */
std::unique_ptr<Socket> Accept(const Socket& listen_socket);
/**
 *  Accepts a connection and creates a new socket.
 *  @param listen_socket A socket that is currently listening for new
 *                       connections
 *  @param address[out] Address of the client whose connection has been 
                        accepted.
 *  @return A new socket that a connection has been accepted.
 *  @exception AcceptException
 */
std::unique_ptr<Socket> Accept(const Socket& listen_socket, 
                               std::shared_ptr<sockaddr_in> address);
	
/**
 *  From Brian "Beej Jorgensen" Hall, Version 3.0.20 March 11, 2016
 *  @link http://beej.us/guide/bgnet/examples/client.c:
 */ 
void *get_in_addr(SOCKADDR *sa);
std::string AddressToString(const sockaddr_in&);
/**
 *  @param sa[in/out] The address that port will be changed.
 *  @param port The new port.
 */
void SetAddressPort(sockaddr_in& sa, u_short port);
}//namespace network
