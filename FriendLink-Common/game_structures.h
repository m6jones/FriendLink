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
*   The point of this file is to build basic structures contianing game data and
*   prepare that data for network transfer.
*/
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "DataHandling.h"

/** 
*   Game related content.
*/
namespace Game {
/**
*   Character Properties.
*/
namespace Property {
///Is the number of properties in PropertyType
///@see Game::Data::PropertyType
constexpr uint8_t kTypeCount = 7;
///The different properties for a character that will be sent over the network.
enum class Type {
  kId,                //uint8 server slot.
  ksCellName,         //String of the cell name.
  kStatus,            //ConnectionSatus of players.
  ksName,             //String of the player name.
  kLocation,          //Location object
  ksWorldSpaceName,   //String of the world space name.
  kLoadedState        //Loaded State  
  //If adding more properties remember to increase kPropertyTypeCount or the 
  //new property will not work.
};
/**
 *  Converts a property type into a string. The strings may not be unique and
 *  some might return "Unknown".
 */
std::string TypeToString(Type);
/**
 *  Packs a property type into a unique char based on conversion to type uint8_t.
 *  @throw runtime_error
 */
char PackType(Type);
/**
 *  Unpacks a char into property type.
 *  @param After c is converted to uint8_t it must be in the range of 0 
 *         to 1-kPropertyTypeCount.
 *  @throw runtime_error
 *  @see Game::Data::kPropertyTypeCount
 */
Type UnpackType(char c);
/**
 *  A structure to pack different character properties.
 */
struct Property {
	Type type;
	std::vector<char> value;
};
/** 
 *  Uses stream syntax to pack and unpack properties to vector<char>.
 *  Example:
 *  @code
 *    std::vector<char> properties { ... } //packed_properties
 *    PropertyStream property_stream { properties };
 *    for (Property p; property_stream >> p;) {
 *	     Update(p);
 *    }
 *  @endcode
 *  @see Game::Data::Property
 */
class Stream {
 public:
  Stream() {}
  /**
   *  Constructs Property Stream from a packed properties.
   *  @param packed_properties is a vector<char> that represents a list of 
   *         properties. To form such a vector encode a list using this class.
   */
  Stream(std::vector<char> packed_properties) 
      : properties_packed_(packed_properties) {}
  Stream(Stream&) = default;
  Stream(Stream&&) = default;

  operator bool() {
    if(test) return false;
    if(Empty()) {
      test = true;
    } 
    return true;
  }
  Stream& operator<<(const Stream&);
  Stream& operator<<(const Property&);
  Stream& operator>>(Property&);

  void Clear() { properties_packed_.clear(); }
  bool Empty() const { return properties_packed_.empty(); }
  std::vector<char> Packed() { return properties_packed_; }

 private:
   std::vector<char> properties_packed_;
   bool test = false;
};

/** 
 *  Properties allows you to keep track of one instance of each property type.
 *  This class is thread safe by the use of locks. 
 *  @todo Determine if Properties is still required.
 */
class Properties {
public:
	Properties() 
    : properties_(kTypeCount),
      property_contained_(kTypeCount, false) {}
  /**
   *  Updates property with type Property.type with new value.
   */
	void Update(Property);
  /**
   *  Updates all properties in the stream with the new values.
   */
  void Update(Stream);
  /**
   *  Updates all properties in the packed with the new values.
   *  @param properties is required to have PacketType properties.
   */
	void Update(Network::Packet::Packet properties);
  
  Stream ToStream();
  /**
   *  Packs all the set properties for sending.
   */
	std::vector<char> Packed();
private:
	std::mutex update_mtx_;
	std::vector<Property> properties_;
	std::vector<bool> property_contained_;
};
/**
 *  Extracts a string from a property where its value represents a string.
 *  @param ks_property with its value of type string. These types have prefex ks.
 */
std::string UnpackString(Property ks_property);
/**
 *  Packs a string into a property where its value represents a string.
 *  @param ks_type that has a prefex of ks.
 */
Property PackString(Type ks_type, std::string);

/**
 *  Unpacks a property to a form id which has type uint32_t.
 *  @param form_id_property with its value of type uint32_t.
 *  @throw runtime_error
 *  @remarks The properties that fillful this property no longer exists.
 */
uint32_t UnpackFormId(Property form_id_property);
/**
 *  Packs a form id which has type uint32_t into a property.
 *  @param type with its value of type uint32_t.
 *  @param form_id is a form id of a object in skyrim.
 *  @remarks The properties that fillful this property no longer exists.
 */
Property PackFormId(Type form_id_type, uint32_t form_id);
}//namespace Property

 /**
 *  Describes a location in skyrim.
 *  A location in skyrim is made up of three parts.
 *
 *  World Space which is null while in interior cells. The world space is
 *  encoded in a form id, UInt32.
 *
 *  Cell which is only null between loading, so the location is considered empty
 *  when the cell is null. The cell is encoded in a form id, UInt32.
 *
 *  World coordinates which is given by normal cartesian coordinates represented
 *  by floats.
 *
 *  Location is ordered by time.
 */
class Location {
  typedef std::chrono::steady_clock SteadyClock;
  typedef std::chrono::duration<int32_t, std::milli> Duration;
  typedef std::chrono::time_point<SteadyClock, Duration> TimePoint;

  static constexpr size_t kDimension = 3;
  static constexpr size_t kLocationTypeSize
    = sizeof(int32_t) + sizeof(char) + 2 * sizeof(uint32_t)
    + kDimension * Network::kSizeOfPackedFloat;
  ///Keeps track of the time that the first Location was constructed.
  static const TimePoint kReferenceTime;

public:
  Location() : has_world_space_(false), has_cell_(false) {}
  /**
  *  Builds location from the location property.
  *  @throw runtime_error if the property isn't of location type.
  */
  explicit Location(Property::Property location_property);
  /**
  *  Builds location from base components.
  *  @param world_space_id is the form id for the world space or false when
  *         there isn't a word space for this location.
  *  @param cell_id is the form id for the cell or false when there isn't a
  *         cell for this location. When this value is false you are creating
  *         location that will be considered empty.
  *  @param position is a vector<float> of length kdimension.
  *  @throw runtime_error if the property isn't of location type.
  *  @see   Game::Data::Location::kDimension
  */
  Location(uint32_t world_space_id, uint32_t cell_id, std::vector<float> position);
  ///@cond LocationConstructors
  //These constructors allow you to input false for world_space_id and cell_id
  //Given the value true for any of the bools will still build the Location as
  //if you gave the value false.
  Location(bool world_space_id, uint32_t cell_id, std::vector<float> position);
  Location(uint32_t world_space_id, bool cell_id, std::vector<float> position);
  Location(bool world_space_id, bool cell_id, std::vector<float> position);
  ///@endcond
  ///Does this location have a world space connected to it.
  bool has_world_space() const { return has_world_space_; }
  ///Does this location have a cell connected to it.
  bool has_cell() const { return has_cell_; }
  ///Does this location give a location in skyrim game.
  bool is_empty() const { return !has_cell_; } //cell cannot be null.
                                               /// @return world space form id. If there is no world space then it returns 0.
  uint32_t world_space_id() const { return world_space_id_; }
  /// @return cell form id. If there is no cell then this returns 0.
  uint32_t cell_id() const { return cell_id_; }
  /// @return The amount of time in milliseconds this location was this created
  ///         after the first location was created.
  int32_t time_elapsed() const { return time_elapsed_; }
  float x() const { return position_[0]; }
  float y() const { return position_[1]; }
  float z() const { return position_[2]; }
  /// @return vector<float> of size 3 that represents the position.
  std::vector<float> position() const { return position_; }
  /// @return A property of type kLocation that represents this location.
  Property::Property ToProperty();
  /// @return A string that represents this location in a readable way.
  std::string ToString();

private:
  /**
  *  Double checks that position is in the right format.
  *  @throw runtime_error
  */
  void CheckPosition(std::vector<float> position);
  /**
  *  Packs the values of has_world_space_ and has_cell_ into one char.
  */
  char PackNotNulls();
  /**
  *  Unpacks a char to sets the values of has_world_space_ and has_cell_.
  */
  void UnpackNotNulls(char c);
  /**
  *  Sets the elapsed time in milliseconds after the first location is created
  *  using kReferenceTime.
  *  @see Game::Data::Location::kReferenceTime
  */
  void SetTimeElapsed();

  bool has_world_space_ = true;
  bool has_cell_ = true;
  std::uint32_t world_space_id_ = 0;
  uint32_t cell_id_ = 0;
  int32_t time_elapsed_ = 0;
  std::vector<float> position_ = {0,0,0};
};
/**
*  Takes in two location and finds the difference in the time created. Note,
*  this is will not return the absolute value. So if location a was created
*  before location b then the result will be negative.
*  @return The difference in time creation in milliseconds or if either of the
*          location is empty then returns Network::Connection::kAntiCongestion.
*  @remark Showing that the default return value is good or bad is required.
*  @see Network::Connection::kAntiCongestion
*/
int TimeSubtract(Location a, Location b);
/**
 *  The distance between two locations.
 *  @return The distance between the two locations. If either location is empty
 *          then 0 is returned.
 */
float DistanceBetween(Location, Location);
/**
 *  Checks to see if the two locations are in the same cell.
 *  @return Returns true if both locations don't have a cell or when their cell
 *          ids match.
 */
bool InSameCell(Location, Location);
/**
*  Checks to see if the two locations are in the same world space.
*  @return Returns true if both locations don't have a world space or when their
*          world space ids match.
*/
bool InSameWorldSpace(Location, Location);
/**
*  Checks to see if the two locations are in the same area. That is if they are
*  in the same cell or if they both have a worldspace and their world space id 
*  match.
*/
bool InSameArea(Location, Location);
/**
*  Extracts a string from a property where its value represents a string.
*/
std::string PrintPosition(Location);

struct LoadedState {
  uint32_t	unk00;	// 00
  uint32_t  unk04;	// 04
  uint32_t	unk08;	// 08
  uint32_t	unk0C;	// 0C
  uint32_t	unk10;	// 10
  uint32_t	unk14;	// 14
  uint32_t	unk18;	// 18
  uint32_t	unk1C;	// 1C
  
  LoadedState() {}
  LoadedState(Property::Property props);
  Property::Property ToProperty();
  std::string ToString();
};
}//namespace Game