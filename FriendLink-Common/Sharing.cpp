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
#include "Sharing.h"
#include "Error.h"
using namespace std;

namespace Data {
namespace Sharing {
//Node
void Node::Write(vector<char> v) {
  value_ = v;
  write_ready_ = false;
}
vector<char> Node::Read(bool& did_read) {
  //If node is being written to tell didRead and return nothing.
  if (is_end() && write_ready()) {
    did_read = false;
    return{};
  }

  vector<char> v(value_.begin(), value_.end());

  write_ready_ = true;
  did_read = true; //Let's not rely on the user initilizing the variable.

  return v;
}
void Node::MarkStart(bool b) {
  is_start_ = b;
}
void Node::MarkEnd(bool b) {
  is_end_ = b;
}
	//Queue
FixedQueue::FixedQueue()
		: data(kDefaultBufferSize) {
	data[0].MarkEnd(true);
	data[0].MarkStart(true);
}
FixedQueue::FixedQueue(size_t buffer_size)
  : data(buffer_size) {
  data[0].MarkEnd(true);
  data[0].MarkStart(true);
}
void FixedQueue::Push(vector<char> vec) {
	write_index_ = MoveEnd(write_index_);
	data[write_index_].Write(vec);
}
void FixedQueue::Push(string str) {
	vector<char> vec(str.begin(), str.end());
	Push(vec);
}
vector<char> FixedQueue::Pop() {
	vector<char> v = data[read_index_].Read(did_read_);
	read_index_ = MoveStart(read_index_);

	return v;
}
int FixedQueue::NextFrom(size_t index) {
	return (index + 1) >= buffer_size() ? 0 : index + 1;	// (index+1) mod bufferSize
}
int FixedQueue::PreviousFrom(size_t index) {
	return (index - 1) < 0 ? buffer_size() - 1 : index -1; // (index-1) mod bufferSize 
}
int FixedQueue::MoveStart(int old_start) {
		
	if (data[old_start].is_end()) {
		return old_start;
	}
	else {
		int new_start = NextFrom(old_start);
		data[new_start].MarkStart(true);
		data[old_start].MarkStart(false);
			
		return new_start;
	}
}
int FixedQueue::MoveEnd(int old_end) {
	int new_end = NextFrom(old_end);
	if (data[new_end].is_start() || data[old_end].write_ready()) {
		return old_end;
	}
	else {
		data[new_end].MarkEnd(true);
		data[old_end].MarkEnd(false);
		return new_end;
	}
}
}//namespace Sharing
}//namespace Data