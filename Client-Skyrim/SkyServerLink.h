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
 *  Handles the connection and information coming from and going to the server.
 */
#pragma once
#include "stdafx.h"
#include <skse\GameReferences.h>
#include "ScriptDragon\skyscript.h"
#include "Client\ServerLink.h"
#include "SkyPlayer.h"

namespace Network {
class SkyServerLink;
/**
 *  This is the gateway that recieves the network data. 
 */
class SkyReceiver : public HandleReceived {
public:
	SkyReceiver() : HandleReceived() {}
	SkyReceiver(SkyServerLink* link) : HandleReceived(), link_(link) {}
  /**
   *  Handles the initial message from the server. 
   */
	void InitialMessageData(InitialMessage);
  /**
   *  Handles when you disconenct from the server.
   */
	void Disconnection();
  /**
   *  Handles when you recieve a packet from the server.
   */
  void PacketData(Packet::Packet);
  /**
   *  Handles error messages
   */
	void ErrorMessage(std::string);
  /**
   *  How many players the server can hold.
   */
	size_t max_players() { return max_players_; }
  /**
   *  The server slot that you referenced too.
   */
	size_t my_server_slot() { return my_server_slot_; }

private:
	SkyServerLink* link_;
	size_t max_players_ = 0;
	size_t my_server_slot_ = 0;
	std::mutex packet_data_mtx_;
};
/**
 *  Manages the connection to the server. When constructed it will initaite the 
 *  connection with the server and when destroyed it will disconnect. While 
 *  connected it manages the information coming in from the server as well as
 *  sending information to the server.
 */
class SkyServerLink {
  ///Skyrim doesn't like being polled to often this is used slow down the 
  ///polling.
  static constexpr int kPlayerDataGatheringDelay = 50; //milliseconds
 public:
   /**
   *  Connects to the server at ip:port (client_port) and defines the base form
   *  for each player.
   *  @param ip The ip adress of the server.
   *  @param port the port for tcp connection and the udp reciever.
   *  @param client_port the port the server uses for udp recieving.
   *  @param traversable_form_id The id of a form with the FLQTranslatorObject
   *                             script attached.
   *  @exception runtime_error when max players is less then server slot. 
   */
  SkyServerLink(std::string ip, std::string port, 
                std::string client_port, UInt32 traversable_form_id);
  /**
   *  Disconnects from the server.
   */
  ~SkyServerLink();

  size_t max_players() { return players_.size(); }
  size_t my_server_slot() { return receiver_.my_server_slot(); }
  ///The skyrim character being played by this client.
  Game::Player::Local& local_player() { return local_player_; }
  ///Removes one of the remote platers.
  void RemovePlayer(size_t index) { players_[index].reset(); }
  /**
   *  Get remote player object that is in server_slot.
   *  @param server_slot The server slot of the remote player. This cannot be
   *                     the local server slot.
   *  @exception runtime_error If server slot is bigger or equal to max players.
   *  @exception runtime_error If server slot is equal to local server slot.                      
   */
  Game::Player::Traversable& server_player(size_t server_slot);
  /**
   *  Start sending and recieving information from the server.
   */
  void StartDataTransfer();
  /**
   *  Checks to see if object is a representation of a player.
   *  @param object A skyrim object that will be compared against the player
   *                objects in players.
   *  @return True if this object is representing a current player.
   */
  bool IsObjectAPlayer(TESObjectREFR* object);
  /**
   *  Marks translating has been complete for object reference.
   */
  void MarkPlayerTranslatingComplete(TESObjectREFR*);
  /**
   *  Send this information when a updated is requested from the server.
   */
  void SendRequestedUpdate();

 private:
  void SendPlayerDataLoop();
  std::vector<std::unique_ptr<Game::Player::Traversable>> players_;
  Game::Player::Local local_player_ {};
  Wsa wsa_startup_;
  SkyReceiver receiver_;
  ServerLink link_;
  UInt32 traversable_form_id_;
  std::thread send_player_data_loop_;
};
/**
 *  Get the ip and port from file FriendLinkIP.cfg. The format of the file is
 *  [ip] [port] [port2]
 *  @param filename filename of the file with the ip and port.
 *  @return A vector of strings, [0]-ip, [1]-port, [2]-port2
 */
std::vector<std::string> GetIpAndPorts(std::string filename);
}//namespace Network