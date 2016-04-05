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
#include "game_structures.h"
#include <sstream>
#include <assert.h>

namespace Game {
namespace Property {
std::string TypeToString(Type property_type) {
  switch (property_type) {
    case Type::kId:
      return "Id";
    case Type::ksName:
      return "Name";
    case Type::ksCellName:
      return "Cell";
    case Type::ksWorldSpaceName:
      return "World Space";
    case Type::kLocation:
      return "Position";
    case Type::kStatus:
      return "Status";
    case Type::kLoadedState:
      return "Loaded States";
    default:
      return "Unknown";
  }
}
char PackType(Type type) {
	return Network::Pack_uint8(uint8_t(type));
}
Type UnpackType(char c) {
	uint8_t x = Network::Unpack_uint8(c);
	if (kTypeCount <= x) {
		throw std::runtime_error("Property type char out of range.");
	}
	return Type(x);
}
Stream& Stream::operator<<(const Stream& stream) {
  properties_packed_.insert(properties_packed_.end(),
                            stream.properties_packed_.begin(),
                            stream.properties_packed_.end());
  return *this;
}
//Enodes a property to a vector<char> that looks like,
//[PropertyType]{char}[size]{uint32_t}[value]{vector<char>(size)}.
//example: Property{ksName, "Amber"} will look like, 4\0\0\05Amber.
Stream& Stream::operator<<(const Property& property) {
  properties_packed_.push_back(PackType(property.type));
  auto size = Network::Pack_uint32(property.value.size());
  properties_packed_.insert(properties_packed_.end(), size.begin(), size.end());
  properties_packed_.insert(properties_packed_.end(), 
                            property.value.begin(), property.value.end());
  return *this;
}
//Decodes a property from a vector<char> that looks like,
//[PropertyType]{char}[size]{uint32_t}[value]{vector<char>(size)}.
//example: 4\0\0\05Amber to Property{ksName, "Amber"}.
Stream& Stream::operator>>(Property& property) {
  if (Empty()) return *this;

  auto iterator = properties_packed_.begin();
  property.type = UnpackType(*iterator);
  iterator += sizeof(char);
  auto size = Network::Unpack_uint32({iterator, iterator + sizeof(uint32_t)});
  iterator += sizeof(uint32_t);
  property.value = {iterator, iterator + size};
  iterator += size;

  properties_packed_.erase(properties_packed_.begin(), iterator);
  return *this;
}
void Properties::Update(Property property) {
	std::lock_guard<std::mutex> lck(update_mtx_);
  property_contained_[uint8_t(property.type)] = true;
  properties_[uint8_t(property.type)] = property;
}
void Properties::Update(Stream property_stream) {
  for (Property property; property_stream >> property;) {
    Update(property);
  }
}
void Properties::Update(Network::Packet::Packet properties) {
	assert(properties.type() == Network::Packet::Type::kProperties 
         && "Add non-properties to a properties via update packet");
	Update(Stream{properties.data()});
}
Stream Properties::ToStream() {
  std::lock_guard<std::mutex> lck(update_mtx_);
  Stream property_stream{};

  for (Property property : properties_) {
    if (property_contained_.at(uint8_t(property.type))) {
      property_stream << property;
    }
  }

  return property_stream;
}
std::vector<char> Properties::Packed() {
	return ToStream().Packed();
}
std::string UnpackString(Property ks_property) {
	return std::string(ks_property.value.data(), ks_property.value.size());
}
Property PackString(Type ks_type, std::string s) {
	std::vector<char> value { s.begin(), s.end() };
	return {ks_type, value };
}
uint32_t UnpackFormId(Property form_id_property) {
  if (form_id_property.value.size() != sizeof(uint32_t)) {
	  throw std::runtime_error("Unpack form ID failed.");
  }
  return Network::Unpack_uint32(form_id_property.value);
}
Property PackFormId(Type form_id_type, uint32_t id) {
	return{ form_id_type, Network::Pack_uint32(id) };
}
}//namespace Property

const Location::TimePoint Location::kReferenceTime
= std::chrono::time_point_cast<Duration>(SteadyClock::now());
//Takes a location property and extracts it as a location.
//The format of this vector<char> is as follows:
//[time_elapsed]{int32}[has_*]{char}[world_space_id]{uint32}[cell_id]{uint32}
//[position]{vector<float>(kDimension)}
//[has_*]{char} format is shown above PackNotNulls() and UnpackNotNulls(c)
Location::Location(Property::Property location_property)
  : position_(kDimension) {
  if (location_property.type != Property::Type::kLocation ||
      location_property.value.size() != kLocationTypeSize) {
    throw std::runtime_error("Location creation failed. Property is wrong.");
  }

  auto property_value = location_property.value.begin();
  time_elapsed_ = Network::Unpack_int32({property_value,
                                        property_value + sizeof(int32_t)});
  property_value += sizeof(int32_t);

  UnpackNotNulls(property_value[0]);
  property_value += sizeof(char);

  world_space_id_ = Network::Unpack_uint32({property_value,
                                           property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  cell_id_ = Network::Unpack_uint32({property_value,
                                    property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  //Position.
  for (int i = 0; i < kDimension; ++i) {
    std::vector<char> coordinate{property_value,
      property_value + Network::kSizeOfPackedFloat};
    position_[i] = Network::Unpack_float754(coordinate);
    property_value += Network::kSizeOfPackedFloat;
  }
}

Location::Location(uint32_t world_space_id, uint32_t cell_id, std::vector<float> position)
  : world_space_id_(world_space_id), cell_id_(cell_id), position_(position) {
  CheckPosition(position);
  SetTimeElapsed();
}
Location::Location(bool world_space_id, uint32_t cell_id, std::vector<float> position)
  : has_world_space_(false),
  cell_id_(cell_id), position_(position) {
  CheckPosition(position);
  SetTimeElapsed();
}
Location::Location(uint32_t world_space_id, bool cell_id, std::vector<float> position)
  : world_space_id_(world_space_id),
  has_cell_(false), position_(position) {
  CheckPosition(position);
  SetTimeElapsed();
}
Location::Location(bool world_space_id, bool cell_id, std::vector<float> position)
  : has_world_space_(false),
  has_cell_(false), position_(position) {
  CheckPosition(position);
  SetTimeElapsed();
}
//Puts location into property by converting the location to a vector<char> of
//size kLocationTypeSize.
//The format of this vector<char> is as follows:
//[time_elapsed]{int32}[has_*]{char}[world_space_id]{uint32}[cell_id]{uint32}
//[position]{vector<float>(kDimension)}
//[has_*]{char} format is shown above PackNotNulls() and UnpackNotNulls(c)
Property::Property Location::ToProperty() {
  std::vector<char> location;
  location.reserve(kLocationTypeSize);

  auto time_elapsed = Network::Pack_int32(time_elapsed_);
  location.insert(location.end(), time_elapsed.begin(), time_elapsed.end());

  location.push_back(PackNotNulls());

  auto world_space_id = Network::Pack_uint32(world_space_id_);
  location.insert(location.end(), world_space_id.begin(), world_space_id.end());

  auto cell_id = Network::Pack_uint32(cell_id_);
  location.insert(location.end(), cell_id.begin(), cell_id.end());

  //Position
  for (float coord : position_) {
    std::vector<char> coordinate = Network::Pack_float754(coord);
    location.insert(location.end(), coordinate.begin(), coordinate.end());
  }

  return{Property::Type::kLocation, location};
}
std::string Location::ToString() {
  return "Time: " + std::to_string(time_elapsed_) + "\n\t" +
    "Has world space: " + std::to_string(has_world_space_) + "\n\t" +
    "Has cell Null: " + std::to_string(has_cell_) + "\n\t" +
    "World Space: " + std::to_string(world_space_id_) + "\n\t" +
    "Cell: " + std::to_string(cell_id_) + "\n\t" +
    "Position: " + std::to_string(x()) +
    "," + std::to_string(y()) +
    "," + std::to_string(z());
}
void Location::CheckPosition(std::vector<float> position) {
  if (position.size() != kDimension) {
    throw std::runtime_error("Location creation failed. Position Vector wrong size.");
  }
}
//Packs has_world_space_ and has_cell_ into a char where the first bit represents
//has_world_space_ and the second bit represents has_cell_. 0-false, 1-true
char Location::PackNotNulls() {
  char c = 0x0;
  if (has_world_space_) c |= 0x2;
  if (has_cell_) c |= 0x1;

  return c;
}
//Unpacks a char where the first bit represents has_world_space_ and the second
//represents has_cell_. 0-false, 1-true
void Location::UnpackNotNulls(char c) {
  if ((c & 0x2) == 0x2) has_world_space_ = true;
  else                  has_world_space_ = false;
  if ((c & 0x1) == 0x1) has_cell_ = true;
  else                  has_cell_ = false;
}
void Location::SetTimeElapsed() {
  auto now = SteadyClock::now();
  time_elapsed_ = std::chrono::duration_cast<Duration>(now - kReferenceTime).count();
}
int TimeSubtract(Location lhs, Location rhs) {
  return !lhs.is_empty() && !rhs.is_empty() ?
    lhs.time_elapsed() - rhs.time_elapsed()
    : Network::kAntiCongestion; //default value, cannot be 0.
}
float DistanceBetween(Location a, Location b) {
  if(a.is_empty() || b.is_empty()) return 0;
  return sqrt((a.x() - b.x())*(a.x() - b.x()) +
              (a.y() - b.y())*(a.y() - b.y()) +
              (a.z() - b.z())*(a.z() - b.z()));
}
bool InSameCell(Location a, Location b) {
  return a.has_cell() == b.has_cell() &&
         (!a.has_cell() || a.cell_id() == b.cell_id());
}
bool InSameWorldSpace(Location a, Location b) {
  return a.has_world_space() == b.has_world_space() &&
         (!a.has_world_space() || a.world_space_id() == b.world_space_id());
}
bool InSameArea(Location a, Location b) {
  return InSameCell(a, b) ||
         (a.has_world_space() && b.has_world_space() &&
          a.world_space_id() == b.world_space_id());
}
std::string PrintPosition(Location location) {
  std::ostringstream oss{};
  oss << '(' << location.x() << ',' <<
    location.y() << ',' <<
    location.z() << ')';
  return oss.str();
}
LoadedState::LoadedState(Property::Property props) {
  auto property_value = props.value.begin();
  unk00 = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk04 = Network::Unpack_uint32({property_value,
                                  property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk08 = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk0C = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk10 = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk14 = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk18 = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);

  unk1C = Network::Unpack_uint32({property_value,
                                 property_value + sizeof(uint32_t)});
  property_value += sizeof(uint32_t);
}

Property::Property LoadedState::ToProperty() {
  std::vector<char> loadedState;
  loadedState.reserve(8 * sizeof(uint32_t));

  auto vunk00 = Network::Pack_uint32(unk00);
  loadedState.insert(loadedState.end(), vunk00.begin(), vunk00.end());

  auto vunk04 = Network::Pack_uint32(unk04);
  loadedState.insert(loadedState.end(), vunk04.begin(), vunk04.end());

  auto vunk08 = Network::Pack_uint32(unk08);
  loadedState.insert(loadedState.end(), vunk08.begin(), vunk08.end());

  auto vunk0C = Network::Pack_uint32(unk0C);
  loadedState.insert(loadedState.end(), vunk0C.begin(), vunk0C.end());

  auto vunk10 = Network::Pack_uint32(unk10);
  loadedState.insert(loadedState.end(), vunk10.begin(), vunk10.end());

  auto vunk14 = Network::Pack_uint32(unk14);
  loadedState.insert(loadedState.end(), vunk14.begin(), vunk14.end());

  auto vunk18 = Network::Pack_uint32(unk18);
  loadedState.insert(loadedState.end(), vunk18.begin(), vunk18.end());

  auto vunk1C = Network::Pack_uint32(unk1C);
  loadedState.insert(loadedState.end(), vunk1C.begin(), vunk1C.end());

  return {Property::Type::kLoadedState, loadedState};
}
std::string LoadedState::ToString() {
  return std::to_string(unk00) + "," +
         std::to_string(unk04) + "," +
         std::to_string(unk08) + "," +
         std::to_string(unk0C) + "," +
         std::to_string(unk10) + "," +
         std::to_string(unk14) + "," +
         std::to_string(unk18) + "," +
         std::to_string(unk1C);
}
}//namespace Game