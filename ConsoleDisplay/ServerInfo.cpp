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
#include <assert.h>
#include <mutex>
#include <algorithm>
#include "ServerInfo.h"
#include "FriendLink-Common\Error.h"

using namespace std;

namespace Display {
mutex Window::kConsoleWriteMtx;
Window::Window(int nlines, int ncols, int beginY, int beginX)
	: window_(newwin(nlines, ncols, beginY, beginX)) { }
	
Window::~Window() {
	if (window_ != nullptr) {
		if (has_colors() == TRUE) {
			wattroff(window_, COLOR_PAIR(color_));
		}
		delwin(window());
	}
}
void Window::Move(int y, int x) {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	mvwin(window(), y, x);
}
void Window::Resize(int nlines, int ncols) {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	wresize(window(), nlines, ncols);
}
void Window::MoveCursor(int y, int x) {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	wmove(window(), y, x);
}
vector<int> Window::GetCursor() const {
	return{ getcury(window_), getcurx(window_) };
}
void Window::PrintLine(int y, string print) {
	print.resize(Dimensions()[1], ' ');
	lock_guard<mutex> lck(kConsoleWriteMtx);
	mvwprintw(window(), y, 0, print.c_str());
}
void Window::Print(string print) {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	wprintw(window(), print.c_str());
}
void Window::Print(int y, int x, string print) {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	mvwprintw(window(), y, x, print.c_str());
}
void Window::Print(int y, int x, int width, string print) {
	print.resize(width, ' ');
	lock_guard<mutex> lck(kConsoleWriteMtx);
	mvwprintw(window(), y, x, print.c_str());
}
vector<int> Window::Dimensions() const {
	return{ getmaxy(window_), getmaxx(window_), 
          getbegy(window_), getbegx(window_) };
}
void Window::Refresh() {
	lock_guard<mutex> lck(kConsoleWriteMtx);
	wmove(window(), 0, 0);
	wrefresh(window());
}
void Window::SetColor(TextColor color) {
	if (has_colors() == TRUE) {
		wattroff(window_, COLOR_PAIR(color_));
		color_ = (int)color;
		wattron(window_, COLOR_PAIR(color_));
	}
}
Table::Table(vector<Game::Property::Type> headers, int beginY, int beginX) {
	int pixel_left = COLS - beginX;
	for (size_t i = 0; i < headers.size(); ++i) {
		int cols_left = headers.size() - i;
    Game::Property::Type column_type = headers[i];
    
    //Make and add column to table
		int pixels = min<int>(pixel_left / cols_left, MaxColumnSize(column_type)+2); //+2 for box
		auto column = std::shared_ptr<Window>( new
        Window{kHeaderLineSize, pixels, beginY, COLS - pixel_left});
    table_.push_back(column);

		//Index the Property Type with column index
		headers_[column_type] = i;

		//Format column
		box(table_[i]->window(), 0, 0);
		table_[i]->Print(1, 1, Game::Property::TypeToString(column_type).c_str());
		mvwhline(table_[i]->window(), 2, 1, 0, pixels-2);
		table_[i]->Refresh();

		pixel_left -= pixels;
	}
}
void Table::SetNumberOfClients(int number_of_clients) {
	for (shared_ptr<Window> column : table_) {
		auto dimensions = column->Dimensions();
		auto rows = number_of_clients + kHeaderLineSize;
		
    //Clear old boxes
		for (int i = min(dimensions[0], rows) - 1; 
         i < max(dimensions[0], rows); ++i) {
			column->Print(i, 0, dimensions[1], " ");
		}
		column->Refresh();

		//Resize and Box it back up!
		column->Resize(rows, dimensions[1]);
		box(column->window(), 0, 0);
    column->Refresh();
	}
}
void Table::Input(Game::Property::Type column_type, size_t slot, string data) {
	if (headers_.count(column_type) == 0) return; //property not being shown.
  int row = slot + kHeaderLineSize - 1;
  int column = headers_.at(column_type);
  auto dimensions = table_[column]->Dimensions();
	table_[column]->Print(row, 1, dimensions[1]-2, data); //1, -2 prints inside box
	table_[column]->Refresh();
}
void Table::Clear(size_t client_slot) {
  int row = client_slot + kHeaderLineSize - 1;
	for (shared_ptr<Window> column : table_) {
		auto dimension = column->Dimensions();
		column->Print(row, 1, dimension[1]-2, " "); //1, -2 prints inside box
		column->Refresh();
	}
}
vector<int> Table::Dimensions() const {
  return {table_.at(0)->Dimensions().at(0),
          COLS, 
          table_.at(0)->Dimensions().at(2),
          table_.at(0)->Dimensions().at(3)};
}
int Table::MaxColumnSize(Game::Property::Type property_type) {
	switch (property_type) {
	  case Game::Property::Type::ksName:
	  case Game::Property::Type::ksCellName:
	  case Game::Property::Type::ksWorldSpaceName:
		  return kColumnSizeString;
  	default:
		  return COLS;
	}
}
size_t PdCursesInitiate::count = 0;
PdCursesInitiate::PdCursesInitiate() {
	if (count == 0) {
		initscr();
		curs_set(0);
		cbreak();
		noecho();
    InitiateTextColor();
		refresh();
	}
	++count;
}
PdCursesInitiate::~PdCursesInitiate() {
	--count;
	if (count == 0) {
		clear();
		endwin();
	}
}
void PdCursesInitiate::InitiateTextColor() {
	if (has_colors() == TRUE) {
		start_color();
		init_pair(short(TextColor::white), COLOR_WHITE, COLOR_BLACK); 
		init_pair(short(TextColor::cyan), COLOR_CYAN, COLOR_BLACK);
		init_pair(short(TextColor::red), COLOR_RED, COLOR_BLACK);
		init_pair(short(TextColor::green), COLOR_GREEN, COLOR_BLACK);
		init_pair(short(TextColor::magenta), COLOR_MAGENTA, COLOR_BLACK);
		init_pair(short(TextColor::yellow), COLOR_MAGENTA, COLOR_BLACK);
	}
}

ServerInfo::ServerInfo(vector<Game::Property::Type> columns)
    : pd_curses_(),
      title_window_(1, COLS, 1, 0),
      client_count_window_(1, COLS, 2, 0),
      clients_table_(columns, 3, 0),
      error_window_(1, COLS, kErrorWindowYBegin, 0) {
	mvprintw(0, 0, kTopMessage);
  error_window_.SetColor(TextColor::red);
	wattron(error_window_.window(), A_BOLD);
	refresh();
}
void ServerInfo::SetClientMax(size_t max_clients) {
  max_clients_ = max_clients;
  clients_table_.SetNumberOfClients(max_clients);
	if(max_clients == client_count_) {
    client_count_window_.SetColor(TextColor::red);
  } else {
    client_count_window_.SetColor(TextColor::green);
  }
  string player_count_text = "Player Count: "
                           + to_string(client_count_) 
                           + "/" + to_string(max_clients);
  client_count_window_.PrintLine(0, player_count_text);
  client_count_window_.Refresh();

	auto old_error_dimensions = error_window_.Dimensions();
  auto clients_dimension = clients_table_.Dimensions();
  error_window_.Move(kErrorWindowYBegin + max_clients, 0);
  error_window_.Refresh();
	if (old_error_dimensions[2] > clients_dimension[0] + clients_dimension[2] || 
      old_error_dimensions[2] < clients_dimension[2]) {
		Window tempWin{old_error_dimensions[0],
                   old_error_dimensions[1],
                   old_error_dimensions[2],
                   old_error_dimensions[3] };
		tempWin.PrintLine(0, " ");
		tempWin.Refresh();
	}
}
void ServerInfo::SetClientCount(size_t  clients_count) {
	if (clients_count > max_clients_)  {
    throw runtime_error("Clients passed Max capactiy in ServerInfo Display");
  }
  client_count_ = clients_count;
	if (max_clients_ == client_count_) {
    client_count_window_.SetColor(TextColor::red);
  } else { 
    client_count_window_.SetColor(TextColor::green);
  }
  string player_count_text = "Player Count: "
    + to_string(client_count_)
    + "/" + to_string(max_clients_);
  client_count_window_.PrintLine(0, player_count_text);
  client_count_window_.Refresh();
}
void ServerInfo::SetTitle(std::string s) {
  title_window_.PrintLine(0, s);
  title_window_.Refresh();
}
void ServerInfo::PrintError(std::string s) {
  error_window_.PrintLine(0, s);
  error_window_.Refresh();
}
}//namespace Display