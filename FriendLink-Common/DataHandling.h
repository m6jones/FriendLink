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
 *  Deals with the raw data coming and leaving through the network.
 */
#pragma once
#include <vector>
#include "Connection.h"

namespace Network {
namespace Packet {
constexpr size_t kSizeOfType = 6;
///Defines the different type of packets.
enum class Type { 
  kUndefined, 
  kInitialMessage,
  kProperties, 
  kStatus,
  kSocketDisconnect,
  kDataRequest
  //If adding more packet types remember to increase kSizeOfType or the 
  //new type will not work.
};
Type UnpackType(char);
char PackType(Type);

static constexpr uint16_t kUniqueCode = 25655;
static constexpr uint32_t kHeaderSize = sizeof(uint32_t) //size of method
                                      + 2 * sizeof(char); //type and slot
/**
 *  Handles packets.
 */
class Packet {
 public:
  /**
   *  @param type The packet type.
   *  @param client The index of the client that this packet belongs too. 
   *                client = max_clients then the server is the owner.
   *  @param data The data the packet holds already packaged.
   */
	Packet(Type type, uint8_t client, std::vector<char> data) 
    : type_(type), client_(client), data_(data) {}
  /**
   *  @param packed_packet A vector<char> that represents a packed_packet.
   *  @exception runtime_error Initializer code did not match.
   */
  Packet(std::vector<char> packed_packet);
	Type type() const { return type_; }
	uint8_t client() const { return client_; }
	const std::vector<char>&  data() const { return data_; }
	uint32_t DataSize() const { return data().size(); }
	uint32_t PackedSize() const { return kHeaderSize +DataSize(); }
	std::vector<char> Packed() const;
 
 private:
	Type type_;
	uint8_t client_ = 0;
	std::vector<char> data_;
	std::vector<char> Header() const;
};

void Send(Socket&, Packet);
Packet Receive(Socket&);

constexpr size_t kSizeOfStatus = 3;
/// These are the states of the property kStatus. 
enum class Status { kNew, kActive, kDisconnected };
/**
 *  Unpacks a status packet into a status.
 *  @param status_packet of type Status
 *  @throw runtime_error When status packet is another type of packet or if the
 *                       status is out of range.
 */
Status UnpackStatus(Packet status_packet);
/**
 *  Packs a status into a packet with client_index.
 *  @param client_index The index of the client this packet represents.
 *  
 */
Packet PackStatus(uint8_t client_index, Status);
/**
 *  Makes a packet that requests data from all clients to send to client_index.
 *  @param client_index Owner of the request.
 */
Packet RequestData(uint8_t client_index);
}//namespace Packet
/**
 *  To organize the intial message from the server to a client.
 */
class InitialMessage {
 public:
  /**
   *  @param max_clients The maxium number of clients the server will accept.
   *  @param client_slot The server slot that the receiving client will be
   *                     represented by.
   */
	InitialMessage(uint8_t max_clients, uint8_t client_slot) 
      : max_clients_(max_clients), client_slot_(client_slot) {}
  /**
   *  @param initial_message_packet Packet with type kInitialMessage.
   *  @exception runtime_error Packet has wrong type.
   *  @exception InitialMessageException 
   */
	InitialMessage(Packet::Packet initial_message_packet);
  /**
   *  Did the initial message report an accepted connection.
   */
  bool ConnectionAccepted() { return client_slot_ <= max_clients_; }
	uint8_t maxClients() const { return max_clients_; }
	uint8_t clientIndex() const { return client_slot_; }
  /**
   *  Packs the initial message into a packet ready to send.
   *  @return a packet of type kInitialMessage
   */
	Packet::Packet Packed() const;
 
 private:
	uint8_t max_clients_;
	uint8_t client_slot_;
};
void Send(Socket&, InitialMessage);

char Pack_uint8(uint8_t a);
uint8_t Unpack_uint8(char c);
std::vector<char> Pack_uint16(uint16_t a);
uint16_t Unpack_uint16(std::vector<char> c);
std::vector<char> Pack_uint32(uint32_t);
uint32_t Unpack_uint32(std::vector<char>);
std::vector<char> Pack_int32(uint32_t);
uint32_t Unpack_int32(std::vector<char>);
std::vector<char> Pack_uint64(uint64_t);
uint64_t Unpack_uint64(std::vector<char>);

constexpr size_t kSizeOfPackedFloat = sizeof(uint32_t);
std::vector<char> Pack_float754(float);
float Unpack_float754(std::vector<char>);	
}