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
#include "stdafx.h"
#include <sstream>
#include "FriendLink-Common\Connection.h"
#include "FriendLink-Common\Error.h"
#include "ConsoleDisplay\Console.h"
#include "Clients.h"

using namespace std;

constexpr size_t kDefaultMaxClients = 6;
constexpr auto kDefaultServerName = "FriendLink Server";

void Help(std::string name) {
  cerr << "Usage: " << name << " [option(s)]\n"
    << "Options:\n"
    << "\t-h,--help,/?\t\tShow this help message\n"
    << "\t-n,--name server name\tSets the server name\n"
    << "\t-mp,--max_players [0-255]\tSets the max number of players allowed on the server.\n"
    << "\t-p1,--port1 port\tSets port the server(tcp) and client(udp) will listen on. \n"
    << "\t-p2,--port2 port\tSets port the server(udp) will listen on\n"
    << endl;
}

struct Arguments {
  string server_name = kDefaultServerName;
  size_t max_clients = kDefaultMaxClients;
  string client_receiver = Network::kDefaultPortClientReceiver;
  string server_receiver = Network::kDefaultPortServerReceiver;
};
/**
 *  @return 0 - run normal, 1 - no errors, 2 - errors
 */
int SetArguments(Arguments& args, int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if ((arg == "-h") || (arg == "--help") || (arg == "/?")) {
      Help(argv[0]);
      return 1;
    } else if ((arg == "-n") || (arg == "--name")) {
      args.server_name = argv[++i];
    } else if ((arg == "-mp") || (arg == "--max_players")) {
      istringstream iss {argv[++i]};
      iss >> args.max_clients;
      if(args.max_clients > 255) { 
        cerr << "Error: max players must be between 0 and 255\n";
        return 1;
      }
    } else if ((arg == "-p1") || (arg == "--port1")) {
      args.client_receiver = argv[++i];
    } else if ((arg == "-p2") || (arg == "--port2")) {
      args.server_receiver = argv[++i];
    }
  }
  return 0;
}

void StartUp(Arguments& args) {
	Error::ClearLog();
	Display::Console::Setup(args.server_name, 
                          args.max_clients,
                          {{Game::Property::Type::ksName,
                            Game::Property::Type::ksWorldSpaceName,
                            Game::Property::Type::ksCellName,
                            Game::Property::Type::kLocation}});
}

void Loop(Arguments& args) {
  Network::Listen listener {args.max_clients, 
                            args.client_receiver,
                            args.server_receiver};

	char q;
	while (q = getch()) {
		if (q == 'q') {
			break;
		}
	}
}

int main(int argc, char* argv[]) {
  Arguments args {};
  int error = SetArguments(args, argc, argv);
  if(error == 1) return 0;
  if(error == 2) return 1;
	StartUp(args);
	try {
		Loop(args);
	} catch (runtime_error& err) {
		Display::Console::PrintError(err.what());
		Error::LogToFile(err);
    return 1;
	} catch (...) {
		Error::LogToFile("Unknown Error");
    return 1;
	}
  return 0;
}

