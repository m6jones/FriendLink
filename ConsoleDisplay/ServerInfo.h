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
 *  Using pdcursers to make a UI for the server.
 */
#pragma once
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
/*
CHANGE in curses.h
MOUSE_MOVED name changed to MOUSE_MOVED_PDC
*/
#include "include/curses.h"
#include "FriendLink-Common/game_structures.h"

namespace Display {
enum class TextColor { white = 1, green, cyan, red, magenta, yellow };
/**
 *  Resource management wrapper for the WINDOW class in curse.h.
 *  Implements common methods that are used on windows as well as an all access
 *  pass to the window. 
 */
class Window {
  static std::mutex kConsoleWriteMtx;
 public:
  /**
   *  Creates a window within the console screen.
   *  @param nlines The number of lines the window will span. 
   *  @param ncols The number of coloumns the window will span.
   *  @param beginY The line where the top of the window will be positioned.
   *  @param beginX The column where the left of the window will be positioned.
   */
	Window(int nlines, int ncols, int beginY, int beginX);
	Window(Window&) = delete;
	Window(Window&&) = delete;
	~Window();
  /**
   *  Gets the WINDOW* so you can use pdcurses functions on the window.
   */
	WINDOW* window() { return window_; }
  /**
   *  Moves the window the the new y,x coordinates.
   *  @param y The line where the top of the window will be positioned.
   *  @param x The column where the left of the window will be positioned.
   */
	void Move(int y, int x);
  /**
   *  Resizes the window.
   *  @param nlines The number of lines the window will span. 
   *  @param ncols The number of coloumns the window will span.
   */
	void Resize(int nlines, int ncols);
  /**
   *  Moves the cursor to a new position
   *  @param y The line number.
   *  @param x The column number.
   */
	void MoveCursor(int y, int x);
  /**
   *  Gets the cursor position.
   *  @return A vector of integers, [0]-line number (y), [1]-column number (x).
   */
	std::vector<int> GetCursor() const;
  /**
   *  Clears line y, then prints the string.
   *  @param y The line to print the string on. Relative to the window.
   */
	void PrintLine(int y, std::string);
  /**
   *  Prints string to current cursor position.
   */
	void Print(std::string);
  /**
   *  Prints string starting at line y and column x relative to window.
   *  @param y The line to print the string on. Relative to the window.
   *  @param x Starting column for printing the string. Relative to the window.
   */
	void Print(int y, int x, std::string);
  /**
   *  Prints string starting at line y and column x relative to window. Insures
   *  that ncols are printed. If the string is smaller then ncols then spaces
   *  fill the string to match the size. 
   *  @param y The line to print the string on. Relative to the window.
   *  @param x Starting column for printing the string. Relative to the window.
   *  @param ncols The number of columns that will be printed.
   */
	void Print(int y, int x, int ncols, std::string);
  /**
   *  Gets the dimensions of the window.
   *  @return A vector<int>, where the values represent,
   *          {nline, ncols, beginY, beginX}
   */
	std::vector<int> Dimensions() const;
  /**
   *  Refreshes the window and moves the cursor to 0,0
   */
	void Refresh();
  /**
   *  If color is available then sets window text color to TextColor.
   */
	void SetColor(TextColor);

 private:
	WINDOW* window_ = nullptr;
	int color_ = 0;
};
/**
 *  Creates a table that shows client information.
 */
class Table {
	static constexpr int kColumnSizeInt = 2;
	static constexpr int kColumnSizeString = 10;
  static constexpr int kHeaderLineSize = 4;
 public:
  /**
   *  @param headers The properties that the table will show.
   *  @param beginY The line where the top of the table will be positioned.
   *  @param beginX The column where the left of the table will be positioned.
   */
	Table(std::vector<Game::Property::Type> headers, 
        int beginY, int beginX);
  /**
   *  Sets the number of clients that the table can hold.
   *  @param number_of_clients The number of clients the table can hold.
   */
	void SetNumberOfClients(int number_of_clients);
  /**
   *  Inputs new data to the table.
   *  @param column The type of data being updated.
   *  @param client_slot The client who this data belongs.
   */
	void Input(Game::Property::Type column, size_t client_slot, std::string);
  /**
   *  Clears the row that is represetned by client_slot.
   *  @param client_slot The row in the table to clear.
   */
	void Clear(size_t client_slot);
  /**
   *  Gets the dimensions of the table.
   *  @return A vector<int>, where the values represent,
   *          {nline, ncols, beginY, beginX}
   */
	std::vector<int> Dimensions() const;
 
 private:
   /**
    * Gets the max column size for the given property type.
    * @return Maxium size for a column of a given property type.
    */
  int MaxColumnSize(Game::Property::Type);
	std::map<Game::Property::Type, int> headers_;
	std::vector<std::shared_ptr<Window>> table_;
	
};
/**
 *  A wrapper to manage the life of pdcurses.
 */
class PdCursesInitiate {
	static size_t count;
public:
  PdCursesInitiate();
  PdCursesInitiate(PdCursesInitiate&) = delete;
  PdCursesInitiate(PdCursesInitiate&&) = delete;
	~PdCursesInitiate();
private:
	void InitiateTextColor();
};
/**
 *  The class that combines all elements to show and handle the display of 
 *  server information.
 */
class ServerInfo {
	static constexpr auto kTopMessage = "Friend Link Server";
  static constexpr auto kErrorWindowYBegin = 7;
 public:
  /**
   *  Setups the console to show and handle the display of server information.
   *  @param column_headers The column headers for the client table. 
   */
	ServerInfo(std::vector<Game::Property::Type> column_headers);
	ServerInfo(ServerInfo&) = delete;
	ServerInfo(ServerInfo&&) = delete;

  /**
   *  Sets the max number of clients for the server.
   */
	void SetClientMax(size_t);
  /**
   *  Sets the current number of clients connected to the server.
   *  @exception runtime_error if the connected_clients are more then the max.
   */
	void SetClientCount(size_t);
  /**
   *  Adds one to the current number of connected clients.
   */
	void AddOneClient() { SetClientCount(client_count_ + 1); }
  /**
   *  Subtracts one from the current number of clients.
   */
	void SubtractOneClient() { SetClientCount(client_count_ - 1); }
	void SetTitle(std::string);
	void PrintError(std::string);
	Table& clients_table() { return clients_table_; }

 private:
  PdCursesInitiate pd_curses_;
	Window title_window_;
	Window client_count_window_;
	Table clients_table_;
	Window error_window_;
  size_t client_count_ = 0;
  size_t max_clients_ = 0;
};
}//namespace Display