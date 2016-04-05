#include "Console.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <FriendLink-Common\Error.h>
#include <FriendLink-Common\Sharing.h>

namespace Display {
namespace Console {
namespace {
/**
 *  This class is important since it manages the order of destruction.
 */
class Console {
  typedef std::vector<std::shared_ptr<Data::Sharing::FixedQueue>> VecSharQueue;
public:
  Console(std::string name,
          size_t max_clients,
          std::vector<Game::Property::Type> columns);
  ~Console();
  void AddReceivedData(Network::Packet::Packet);
  void AddReliableReceivedData(Network::Packet::Packet);
  void PrintError(std::string error);

 private:
  void UpdateProperty(size_t client_index, Game::Property::Property);
  void Update();
  bool UpdateFromData(VecSharQueue& data);
  ServerInfo server_info_;
  VecSharQueue data_;
  VecSharQueue reliable_data_; //TODO Better data structure required.
  std::thread update_console_;
  std::atomic<bool> console_on_ = true;
};
Console::Console(std::string name,
                 size_t max_clients,
                 std::vector<Game::Property::Type> columns)
    : server_info_(columns) {
  server_info_.SetClientMax(max_clients);
  server_info_.SetTitle(name);

  data_.resize(max_clients);
  reliable_data_.resize(max_clients);
  for (size_t i = 0; i < max_clients; ++i) {
    data_[i] = std::make_shared<Data::Sharing::FixedQueue>();
    reliable_data_[i] = std::make_shared<Data::Sharing::FixedQueue>();
  }
  update_console_ = std::thread(&Console::Update, this);
}
Console::~Console() {
  console_on_ = false;
  update_console_.join();
}
void Console::AddReceivedData(Network::Packet::Packet packet) {
  if (packet.client() >= data_.size()) {
    throw std::runtime_error("Client Index passed Max capactiy can't input data.");
  }
  data_[packet.client()]->Push(packet.Packed());
}
void Console::AddReliableReceivedData(Network::Packet::Packet packet) {
  if (packet.client() >= reliable_data_.size()) {
    throw std::runtime_error("Client Index passed Max capactiy can't input data.");
  }
  reliable_data_[packet.client()]->Push(packet.Packed());
}
void Console::PrintError(std::string error) {
  server_info_.PrintError(error);
}
void Console::UpdateProperty(size_t client_index, Game::Property::Property property) {
  Table& clients = server_info_.clients_table();
  switch(property.type) {
    case Game::Property::Type::ksName:
    case Game::Property::Type::ksCellName:
    case Game::Property::Type::ksWorldSpaceName: {
      std::string name = Game::Property::UnpackString(property);
      clients.Input(property.type, client_index, name);
      break; }
    case Game::Property::Type::kLocation: {
      Game::Location location {property};
      clients.Input(property.type, client_index, Game::PrintPosition(location));
      break; }
    case Game::Property::Type::kStatus: {
      
      break; }
    case Game::Property::Type::kLoadedState: {
      auto loadedState = Game::LoadedState(property);
      clients.Input(property.type, client_index, loadedState.ToString());
      break; }
    default: {
      break; }
  }
}
bool Console::UpdateFromData(VecSharQueue& data) {
  bool character_updated = false;
  for (size_t i = 0; i < data.size(); ++i) {
    std::vector<char> packed_packet = data[i]->Pop();
    if(data[i]) {
      auto packet = Network::Packet::Packet(packed_packet);
      if(packet.type() == Network::Packet::Type::kProperties) {
        Game::Property::Stream stream{packet.data()};
        for (Game::Property::Property p; stream >> p;) {
          UpdateProperty(i, p);
        }
      } else if (packet.type() == Network::Packet::Type::kStatus) {
        Table& clients = server_info_.clients_table();
        auto status = Network::Packet::UnpackStatus(packet);
        if (status == Network::Packet::Status::kNew) {
          server_info_.AddOneClient();
        } else if (status == Network::Packet::Status::kDisconnected) {
          server_info_.SubtractOneClient();
          clients.Clear(packet.client());
        }
      }
      character_updated = true;
    }
  }
  return character_updated;
}
void Console::Update() {
  while (console_on_) {
    try {
      if(!UpdateFromData(data_) && !UpdateFromData(reliable_data_)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } catch (std::runtime_error& err) {
      Error::LogToFile(err);
    } catch (...) {
      Error::LogToFile("Unknown: Caught Update Console");
    }
  }
}
//unique pointer used because this is required to be destroyed. There is no code
//that should depend on this being alive.
std::unique_ptr<Console> g_console;
}//namespace
void Setup(std::string name,
           size_t max_clients,
           std::vector<Game::Property::Type> columns) {
  g_console.reset(new Console(name, max_clients, columns));
}
void AddReceivedData(Network::Packet::Packet packet) {
  if(g_console) {
    g_console->AddReceivedData(packet);
  }
}
void AddReliableReceivedData(Network::Packet::Packet packet) {
  if (g_console) {
    g_console->AddReliableReceivedData(packet);
  }
}
void PrintError(std::string error) {
  if (g_console) {
    g_console->PrintError(error);
  }
}
}//namespace Console
}//namespace Display