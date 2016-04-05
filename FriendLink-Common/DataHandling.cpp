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
#include "DataHandling.h"
#include "NetworkExceptions.h"
#include "Error.h"
using namespace std;

namespace Network {
namespace Packet {
Type UnpackType(char packed_type) {
  uint8_t x = Unpack_uint8(packed_type);
  if(x >= kSizeOfType) return Type::kUndefined;
  return Type(x);
}
char PackType(Type type) {
  return Network::Pack_uint8(uint8_t(type));
}
Packet::Packet(vector<char> packed_packet) {
  if(packed_packet.size() == 0) {
    type_ = Type::kSocketDisconnect;
  } else {
    auto iterator = packed_packet.begin();
    vector<char> code = {iterator, iterator + sizeof(kUniqueCode)};
    if(kUniqueCode != Unpack_uint16(code)) {
      throw runtime_error("Initializer code did not match.");
    }
    iterator += sizeof(kUniqueCode);

    vector<char> header;
    header.reserve(kHeaderSize);
    header.insert(header.begin(), iterator, iterator + kHeaderSize);
    iterator += kHeaderSize;

    client_ = Unpack_uint8(header.back());
    header.pop_back(); //get ride of client id

    type_ = UnpackType(header.back());
    header.pop_back(); //get ride of the type

    uint32_t data_size = Unpack_uint32(header);
    vector<char> data;
    data.reserve(data_size);
    data.insert(data.begin(), iterator, packed_packet.end());
  }
}
vector<char> Packet::Packed() const {
  vector<char> packed;
  vector<char> header = Header();
  packed.reserve(PackedSize());
  packed.insert(packed.end(), header.begin(), header.end());
  packed.insert(packed.end(), data().begin(), data().end());

  return packed;
}
vector<char> Packet::Header() const {
  auto header = Pack_uint16(kUniqueCode);
  auto data_size = Pack_uint32(DataSize());
  header.insert(header.begin(), data_size.begin(), data_size.end());
  header.push_back(PackType(type()));
  header.push_back(Pack_uint8(client()));
  return header;
}
void Send(Socket& s, Packet p) {
  Send(s, p.Packed());
}
Packet Receive(Socket& socket) {
  if(!ReceiveUntil(socket, Pack_uint16(kUniqueCode))) {
    return{Type::kSocketDisconnect, 0,{}};
  }
  vector<char> header = Receive(socket, kHeaderSize);
  if (header.empty()) return{Type::kSocketDisconnect, 0, {}};

  uint8_t client = Unpack_uint8(header.back());
  header.pop_back(); //get client id

  Type type = UnpackType(header.back());
  header.pop_back(); //get ride of the type

  uint32_t data_size = Unpack_uint32(header);

  vector<char> data = Receive(socket, data_size);
  if (data.empty()) return{Type::kSocketDisconnect, 0,{}};

  return{type, client, data};
}
Status UnpackStatus(Packet status_packet) {
  if (status_packet.type() != Type::kStatus) {
    throw std::runtime_error("Expected property of type kStatus.");
  }
  uint8_t x = Network::Unpack_uint8(status_packet.data()[0]);
  if (x < 0 || kSizeOfStatus <= x) {
    throw std::runtime_error("Status unpack out of range.");
  }

  return Status(x);
}
Packet PackStatus(uint8_t client_index, Status status) {
  return{Type::kStatus, client_index , {Network::Pack_uint8(uint8_t(status))}};
}
Packet RequestData(uint8_t client_index) {
  return {Type::kDataRequest, uint8_t(client_index),{'0'}};
}
}//namespace Packet

InitialMessage::InitialMessage(Packet::Packet initial_message_packet) {
  if(initial_message_packet.type() != Packet::Type::kInitialMessage) {
    throw runtime_error("Expecting kInitialMessage type packet.");
  }
  auto message = initial_message_packet.data();
  //decodes [maxClient:uint8_t][indexClient:uint8_t]
	if (message.size() != 2) {
		throw InitialMessageException("Packed Initial message wrong length.");
	}

  max_clients_ = Unpack_uint8({ message[0] });
  client_slot_ = Unpack_uint8({ message[1] });
}
Packet::Packet InitialMessage::Packed() const {
	vector<char> message = {Pack_uint8(max_clients_), Pack_uint8(client_slot_)};
	return {Packet::Type::kInitialMessage, max_clients_, message};
}
void Send(Socket& socket, InitialMessage message) {
	Send(socket, message.Packed());
}

namespace {
template<typename integer>
vector<char> EncodeInt(integer a) {
	vector<char> value(sizeof integer);
		
	for (size_t i = 0; i < sizeof integer; ++i) {
		value[i] = (a >> (i * 8)) & 0xFF;
	}
  return value;
}
template<typename integer>
integer DecodeUInt(vector<char> v) {
	integer value = 0;

	for (size_t i = 0; i < sizeof integer; ++i) {
		value |= ((integer)((unsigned char)v[i]) << (i * 8));
	}
  return value;
}
template<typename integer>
//contract: uint32_t, uint8_t, uint64_t
integer DecodeInt(vector<char> v) {
  integer value = 0;

  for (size_t i = 0; i < sizeof integer; ++i) {
    value |= ((integer)(v[i]) << (i * 8));
  }

  return value;
}
}//namespace
char Pack_uint8(uint8_t a) {
	return EncodeInt<uint8_t>(a)[0];
}
uint8_t Unpack_uint8(char c) {
	return DecodeUInt<uint8_t>({ c });
}
vector<char> Pack_uint16(uint16_t a) {
  return EncodeInt<uint16_t>(htons(a));
}
uint16_t Unpack_uint16(std::vector<char> v) {
  return ntohs(DecodeUInt<uint16_t>(v));
}
vector<char> Pack_uint32(uint32_t a) {
	return EncodeInt<uint32_t>(htonl(a));
}
uint32_t Unpack_uint32(std::vector<char> v) {
	return ntohl(DecodeUInt<uint32_t>(v));
}
vector<char> Pack_int32(uint32_t a) {
  return EncodeInt<int32_t>(htonl(a));
}
uint32_t Unpack_int32(std::vector<char> v) {
  return ntohl(DecodeUInt<int32_t>(v));
}
vector<char> Pack_uint64(uint64_t a) {
	return EncodeInt<uint64_t>(htonll(a));
}
uint64_t Unpack_uint64(std::vector<char> v) {
	return ntohll(DecodeUInt<uint64_t>(v));
}
/**
 *  [sign-1][exponent-8][significand-23] - 32 bits
 *  Remark: Implenation influenced by Brian "Beej Jorgensen" Hall, Guide to Network Programming 
 *  @link http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization as of 1/12/2016
 */
vector<char> Pack_float754(float d) {
	uint32_t encoder = 0;
	if (d == 0.0) return Pack_uint32(encoder); // get this special case out of the way
		
	// check sign and begin normalization
	float dnorm = d;
  int32_t sign = 0;
	if (d < 0) { 
		sign = 1; 
		dnorm = -d; 
	}

	// get the normalized form of f and track the exponent
	int shift = 0;
	while (dnorm >= 2.0f) { 
		dnorm /= 2.0f; 
		shift++; 
	}
	while (dnorm < 1.0f) { 
		dnorm *= 2.0f; 
		shift--;
	}
	dnorm = dnorm - 1.0f; //since 1 <= dnorm < 2 so the first digit is always 1. Don't encode that.

	const unsigned int bits = 32;
	const unsigned int expbits = 8;
	const unsigned int significandbits = bits - expbits - 1; // -1 for sign bit

	// calculate the binary form (non-float) of the significand data
  int32_t significand = (int32_t)(dnorm * ((1LL << significandbits) + 0.5f));

	// get the biased exponent
  int32_t exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias

	// the final answer
	encoder = (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;

	return Pack_uint32(encoder);
}
/**
 *  [sign-1][exponent-8][significand-23] - 32 bits
 *  Remark: Implenation influenced by Brian "Beej Jorgensen" Hall, Guide to Network Programming 
 *  @link http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization as of 1/12/2016
 */
float Unpack_float754(vector<char> vec) {
  uint32_t decoder = Unpack_uint32(vec);

	if (decoder == 0) return 0.0;

	// pull the significand
	const unsigned int bits = 32;
	const unsigned int expbits = 8;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	float result = float((decoder&((1LL << significandbits) - 1))); // mask
	result /= (1LL << significandbits); // convert back to float
	result += 1.0f; // add the one back on

	// deal with the exponent
	const uint32_t bias = (1 << (expbits - 1)) - 1;
  int32_t shift = ((decoder >> significandbits)&((1LL << expbits) - 1)) - bias;
	while (shift > 0) { 
		result *= 2.0; 
		shift--; 
	}
	while (shift < 0) { 
		result /= 2.0; 
		shift++; 
	}

	// sign it
	result *= (decoder >> (bits - 1)) & 1 ? -1.0f : 1.0f;

	return result;
}
}//namepsace Network