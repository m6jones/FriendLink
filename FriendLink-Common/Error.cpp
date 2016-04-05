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
#include "Error.h"
#include <fstream>
#include <mutex>
#include <ctime>
using namespace std;
namespace Error {
	void Error(string s) { 
		throw runtime_error(s);
	}
	void ClearLog() {
		static mutex mtx;

		unique_lock<mutex> lck(mtx);
		ofstream ost{ filename };
		if (!ost) return;

		ost << "Log File for FriendLink" << '\n';
	}
	void LogToFile(exception& e) {
		LogToFile(e.what());
	}
	void LogToFile(string s) {
		static mutex mtx;
		// current date/time based on current system
		time_t now = time(0);
		struct tm ltm;
		localtime_s(&ltm, &now);

		unique_lock<mutex> lck(mtx);
		ofstream ost{filename, ofstream::app };
		if (!ost) return;

		ost << '[' << 1900 + ltm.tm_year << '\\' 
			<< 1 + ltm.tm_mon << '\\'
			<< ltm.tm_mday << ' '
			<< ltm.tm_hour << ':'
			<< ltm.tm_min << ':'
			<< ltm.tm_sec << ']'
			<< s << '\n';
	}
	void LogToFile(std::string main, int code, std::string message) {
		LogToFile(main + "\n\t Code: " + to_string(code) + "\n\t Message: " + message);
	}
}