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
#include "SkyServerLink.h"
#include <skse\PapyrusQuest.h>
#include "FriendLink-Common\Error.h"
#include "Client-Skyrim.h"

using namespace std;

namespace Network {

void SkyReceiver::InitialMessageData(InitialMessage init_message) {
  max_players_ = init_message.max_clients();
  my_server_slot_ = init_message.client_index();
}

void SkyReceiver::Disconnection() {
	if (Game::IsGameOn()) {
		auto quest = (ScriptDragon::TESQuest*)
                 papyrusQuest::GetQuest({}, FriendLink::kQuest);
		ScriptDragon::Quest::SetCurrentStageID(quest, 20);
	}
}

void SkyReceiver::PacketData(Packet::Packet packet) {
	unique_lock<mutex> lck(packet_data_mtx_); //tcp recv and udp recv will call this thread.
	switch(packet.type()) {
    case Packet::Type::kProperties: {
      if (packet.client() != my_server_slot_) {
        link_->server_player(packet.client()).SetProperties({packet.data()});
		  }
      break; }
    case Packet::Type::kDataRequest: {
      link_->SendRequestedUpdate();
      break; }
    case Packet::Type::kStatus: {
      auto connection_status = Packet::UnpackStatus(packet);
      if (connection_status == Packet::Status::kDisconnected) {
        link_->RemovePlayer(packet.client());
      }
      break; }
    default: {
      break;
    }
  }
	lck.unlock();
}
void SkyReceiver::ErrorMessage(std::string s) {
	Error::LogToFile(s);
}
SkyServerLink::SkyServerLink(std::string ip, 
                              std::string port,
                              std::string client_port, 
                              UInt32 traversable_form_id)
	: wsa_startup_(), 
    receiver_(this), 
    link_(ip, port, client_port, receiver_),
    traversable_form_id_(traversable_form_id) { 
	link_.ReceiveInitialMessage();
	if (my_server_slot() >= receiver_.max_players()) {
		throw runtime_error("index cannot be bigger then maxPlayers");
	}
	players_.resize(receiver_.max_players());
}
SkyServerLink::~SkyServerLink() {
  link_.Disconnect();
	if (send_player_data_loop_.get_id() != thread::id()) {
    send_player_data_loop_.join();
	}
}
Game::Player::Traversable& SkyServerLink::server_player(size_t slot) {
	if (slot >= max_players()) {
		throw runtime_error("server_player index cannot be bigger then max_players.");
	}
  if(my_server_slot() == slot) {
    throw runtime_error("server_player cannot get local player.");
  }
	if (!players_[slot]) {
		players_[slot].reset(new Game::Player::Traversable(traversable_form_id_));
	}
	return *players_[slot];
}

void SkyServerLink::SendPlayerDataLoop() {
  //Initial Data Send
	{
		auto data = local_player().GetProperties({Game::Property::Type::ksName,
                                              Game::Property::Type::kLocation});
		link_.SendReliable(data);
    link_.SendReliable(data);
		link_.SendDataRequest();
	}
  while (link_.IsActive()) {
	  auto data = local_player().GetProperties({
        Game::Property::Type::ksWorldSpaceName,
			  Game::Property::Type::ksCellName,
        Game::Property::Type::kLocation
	  });
    link_.Send(data);
    this_thread::sleep_for(chrono::milliseconds(kPlayerDataGatheringDelay));
  }
}
void SkyServerLink::StartDataTransfer() { 
  send_player_data_loop_ = thread(&SkyServerLink::SendPlayerDataLoop, this);
	link_.StartDataTransfer();
}
bool SkyServerLink::IsObjectAPlayer(TESObjectREFR* object) {
	for (int i = 0; i < max_players(); ++i) {
		if (players_[i] && players_[i]->CompareObjectTo({object})) {
			return true;
		}
	}
	return false;
}
void SkyServerLink::MarkPlayerTranslatingComplete(TESObjectREFR* object) {
  for (int i = 0; i < max_players(); ++i) {
    if (players_[i] && players_[i]->CompareObjectTo({object})) {
      players_[i]->MarkTranslationComplete();
    }
  }
}
void SkyServerLink::SendRequestedUpdate()
{
	auto baseData = local_player().GetProperties({
        Game::Property::Type::ksName,
        Game::Property::Type::ksWorldSpaceName,
        Game::Property::Type::ksCellName,
        Game::Property::Type::kLocation });
	link_.SendReliable(baseData);
}
vector<string> GetIpAndPorts(std::string filename) {
	ifstream ifs(filename, std::ifstream::in);

	string ip, port, clientPort;
	ifs >> ip >> port >> clientPort;

	return{ ip, port, clientPort };
}
} //namespace Network