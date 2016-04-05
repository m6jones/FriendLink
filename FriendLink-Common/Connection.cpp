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
#include "Connection.h"
#include <ws2tcpip.h>
#include "Error.h"
#include "NetworkExceptions.h"

namespace Network {
// Connection Class
Wsa::Wsa() {
	// Initialize Winsock
	int err = WSAStartup(MAKEWORD(2, 2), &wsa_data_);
	if (err != 0) {
		throw WSADataException(err);
	}
}
Wsa::~Wsa() {
  Error::LogToFile("Connection deleted");
	if (WSACleanup() == SOCKET_ERROR) {
		LogCleanupError(WSAGetLastError());
	}
}
void Wsa::LogCleanupError(int err) {
	std::string wsaError;
	switch (err) {
	case WSANOTINITIALISED:
		wsaError = ErrorMessages::notInitalised;
		break;
	case WSAENETDOWN:
		wsaError = ErrorMessages::eNetDown;
		break;
	case WSAEINPROGRESS:
		wsaError = ErrorMessages::inProgress;
		break;
	default:
		wsaError = ErrorMessages::unkownError;
	}
  Error::LogToFile("WSA Cleanup Error", err, wsaError);
}
AddressHint::AddressHint() {
  ZeroMemory(&hints_, sizeof(hints_));
}
AddressHint::AddressHint(int ai_family, int ai_socktype, 
                         int ai_protocol, int ai_flags) {
  ZeroMemory(&hints_, sizeof(hints_));
  SetFlags(ai_flags);
  SetFamily(ai_family);
  SetSockType(ai_socktype);
  SetProtocol(ai_protocol);
}
AddressHint::AddressHint(int ai_family, int ai_socktype, int ai_protocol) {
  ZeroMemory(&hints_, sizeof(hints_));
  SetFamily(ai_family);
  SetSockType(ai_socktype);
  SetProtocol(ai_protocol);
}
void AddressHint::SetFlags(int ai_flags) {
  hints_.ai_flags = ai_flags;
}
void AddressHint::SetFamily(int ai_family) {
  hints_.ai_family = ai_family;
}
void AddressHint::SetSockType(int ai_socktype) {
  hints_.ai_socktype = ai_socktype;
}
void AddressHint::SetProtocol(int ai_protocol) {
  hints_.ai_protocol = ai_protocol;
}

Address::Address(std::string host, std::string port, const AddressHint& hints) {
	int err = 0;
	if (host.empty()) {
		err = getaddrinfo(NULL, port.c_str(), &hints.get(), &address_);
	} else {
		err = getaddrinfo(host.c_str(), port.c_str(), &hints.get(), &address_);
	}

	if (err != 0) {
		throw AddressException(err);
	}
	ptr_ = address_;
}
Address::~Address() {
	freeaddrinfo(address_);
}
void Address::Next() {
  ptr_ = ptr_->ai_next;
}
bool Address::NotEnd() {
	if (ptr_ == NULL) {
		ptr_ = address_;
		return false;
	}
  return true;
}
Socket::Socket(int ai_family, int ai_socktype, int ai_protocol) {
	socket_ = socket(ai_family, ai_socktype, ai_protocol);
	if (socket_ == INVALID_SOCKET) {
		throw SocketException(WSAGetLastError());
	}
}
Socket::Socket(const addrinfo& address) {
	socket_ = socket(address.ai_family, address.ai_socktype, address.ai_protocol);
	if (socket_ == INVALID_SOCKET) {
		throw SocketException(WSAGetLastError());
	}
}
Socket::~Socket() {
	if (IsValid()) {
		try {
			Close();
		} catch (CloseSocketException e) {
			Error::LogToFile(e.what(), e.code(), e.message());
		}
	}
}
void Socket::Shutdown(int how) {
	if (IsValid()) {
		if (shutdown(socket_, how) == SOCKET_ERROR) {
			throw ShutdownException(WSAGetLastError());
		}
	}
}
void Socket::Close() { 
	if (IsValid()) {
		if (closesocket(socket_) == SOCKET_ERROR) {
			throw CloseSocketException(WSAGetLastError());
		}
		socket_ = INVALID_SOCKET;
	}
}
bool Socket::IsTcp() const {
  char type;
  int length = sizeof(char);
  int error = getsockopt(socket_, SOL_SOCKET, SO_TYPE, &type, &length);
  
  if (error == SOCKET_ERROR) {
    throw SocketOptionException(WSAGetLastError());
  }

  return type == SOCK_STREAM;
}
bool Socket::ReadyToSend() {
  bool result = time_last_sent_ + std::chrono::milliseconds(kAntiCongestion)
                <= std::chrono::steady_clock::now();
  time_last_sent_ = std::chrono::steady_clock::now();
  return result;
}
std::unique_ptr<Socket> BindTo(Address& address) {
	for (auto addr = address.current(); address.NotEnd(); address.Next()) {
		auto socket = std::unique_ptr<Socket>(new Socket(addr));
		if (Bind(addr, *socket)) {
			return std::move(socket);
		}
	}
  //Only the last bind is logged.
	throw BindException(WSAGetLastError());
}
void BindTo(Address& address, Socket& binding_socket) {
	for (auto addr = address.current(); address.NotEnd(); address.Next()) {
		if (Bind(addr, binding_socket)) {
			return;
		}
	}
  //Only the last bind is logged.
	throw BindException(WSAGetLastError());
}
bool Bind(const addrinfo& address, Socket& binding_socket) {
  int error = bind(binding_socket.Get(), address.ai_addr, address.ai_addrlen);
  return error != SOCKET_ERROR;
}
std::unique_ptr<Socket> ConnectTo(Address& address) {
	for (auto addr = address.current(); address.NotEnd(); address.Next()) {
		auto socket = std::unique_ptr<Socket>(new Socket(addr));
		if (Connect(addr, *socket)) {
			return std::move(socket);
		}
	}
  //Only last connection is logged.
	throw ConnectException(WSAGetLastError());
}
void ConnectTo(Address& address, Socket& connecting_socket) {
	for (auto addr = address.current(); address.NotEnd(); address.Next()) {
		if (Connect(addr, connecting_socket)) {
			return;
		}
	}
  //Only last connection is logged.
	throw ConnectException(WSAGetLastError());
}
std::unique_ptr<Socket> ConnectTo(const sockaddr_in& address,
                                  int ai_socktype,
                                  int ai_protocol) {
  auto socket = std::make_unique<Socket>(
      Socket(address.sin_family, ai_socktype, ai_protocol));
  int error = connect(socket->Get(), (SOCKADDR *)&address, sizeof(address));
  if(error != SOCKET_ERROR) {
    throw ConnectException(WSAGetLastError());
  }
  return move(socket);
}
bool Connect(const addrinfo& address, Socket& socket) {
  int error = connect(socket.Get(), address.ai_addr, address.ai_addrlen);
  return error != SOCKET_ERROR;
}
void Send(Socket& socket, std::vector<char> data) {
  if(socket.IsTcp() || socket.ReadyToSend()) {
		const char* sendbuf{ data.data() };
		int len = data.size();
		while(len > 0) {
			int sent = send(socket.Get(), sendbuf+(data.size()-len), len, 0);
			if (sent == SOCKET_ERROR) {
				throw SendException(WSAGetLastError());
			}
			len = len - sent;
		}
  }
}
std::vector<char> Receive(Socket& socket, uint32_t n) {
	while (socket.recieved_data().Length() < n) {
    std::vector<char> recvbuf(kBufferSize);
		int len = recv(socket.Get(), &recvbuf[0], kBufferSize, 0);
		if (len == 0) {
			return{}; //If we recieve 0 bits then send empty vector
		}
		if (len == SOCKET_ERROR) {
			if (WSAECONNABORTED == WSAGetLastError()) {
				return{};
			}
			throw RecvException(WSAGetLastError());
		}
		socket.recieved_data().Push(recvbuf.data(), len);
	}
  return socket.recieved_data().Pop(n);
}
bool ReceiveUntil(Socket& blocking_socket, std::vector<char> code) {
  for(size_t i = 0; i < code.size() && blocking_socket.IsValid();) {
    auto received = Receive(blocking_socket, 1);
    if(received.empty()) {
      return false;
    } else if(received[0] == code[i]) {
      ++i;
    } else {
      i = 0;
    }
  }
  return true;
}
void ListenOn(const Socket& socket) {
	if (listen(socket.Get(), SOMAXCONN) == SOCKET_ERROR) {
		throw ListenException(WSAGetLastError());
	}
}
std::unique_ptr<Socket> Accept(const Socket& listen_socket) {
  auto socket = std::make_unique<Socket>(accept(listen_socket.Get(), NULL, NULL));
	if (!socket->IsValid()) {
		throw AcceptException(WSAGetLastError());
	}
  return move(socket);
}
std::unique_ptr<Socket> Accept(const Socket& listen_socket,
                               std::shared_ptr<sockaddr_in> address) {
	int address_size = sizeof(*address);
  auto socket = std::make_unique<Socket>(
      accept(listen_socket.Get(), (SOCKADDR *)address.get(), &address_size));
	if (!socket->IsValid()) {
		throw AcceptException(WSAGetLastError());
	}
		
	return move(socket);
}

// get sockaddr, IPv4 or IPv6:
//Seen from http://beej.us/guide/bgnet/examples/client.c:
void *get_in_addr(SOCKADDR *sa) {
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
std::string AddressToString(const sockaddr_in& sa) {
  char s[INET6_ADDRSTRLEN];
  inet_ntop(sa.sin_family, get_in_addr(((SOCKADDR *)&sa)), s, sizeof(s));
  std::string result (s,INET6_ADDRSTRLEN);
  result += ":"+std::to_string(ntohs(sa.sin_port));
  return result;
}
void SetAddressPort(sockaddr_in& sa, u_short port) {
  sa.sin_port = htons(port);
}
}//namespace Network
