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
 *  Datasctructures that are made with threading in mind.
 */
#pragma once
#include <vector>
#include <string>
#include <atomic>

namespace Data {
namespace Sharing {
class Node {
 public:
	Node() {};
  /**
   *  Writes the vector to the node.
   */
	void Write(std::vector<char>);
  /**
   *  Trys to read from node. If the node is empty then returns empty vector and
   *  sets did_read to false.
   *  @param did_read[out] Sets to true if the read called read something.
   */
	std::vector<char> Read(bool& did_read);
  /**
   *  Marks if this is the start of the queue.
   */
	void MarkStart(bool);
  /**
   *  Marks if this is the end of the queue.
   */
	void MarkEnd(bool);
	bool is_start() { return is_start_; }
	bool write_ready() { return write_ready_; }
  bool is_end() { return is_end_; }

 private:
	std::atomic<bool> is_start_{ false };
	std::atomic<bool> write_ready_{ true }; //This allows reader to read the end of the queue
	std::atomic<bool> is_end_{ false };
	std::vector<char> value_;
};
/**
 *  The fixed queue has a fixed sized and when filled will replace the last
 *  entire with the new one. This class is thread safe for two threads. One that
 *  reads and one that writes.
 *  @todo Make this a template container.
 */
class FixedQueue {
	static constexpr size_t kDefaultBufferSize = 5;
 public:
  /**
   *  Creates a fixed queue with the default buffer size.
   */
	FixedQueue();
  /**
   *  Creates a fixed queue with the default buffer size.
   *  @param buffer_size The size of the buffer.
   */
  FixedQueue(size_t buffer_size);
	FixedQueue(const FixedQueue&) = delete;
	FixedQueue(const FixedQueue&&) = delete;
  /**
   *  Adds vector to the queue. If the queue is full then it will replace the
   *  element at the end of the queue.
   */
	void Push(std::vector<char>);
  /**
   *  Adds vector to the queue. If the queue is full then it will replace the
   *  element at the end of the queue.
   */
	void Push(std::string);
  /**
   *  Pops the top element off the queue. If there is no element to pop then the
   *  empty vector is returned and the fixed queue bool operator will return
   *  false.
   *  @remark Check the fixed queue bool operator to see if anything has been
   *          popped.
   */
	std::vector<char> Pop();
  /**
   *  Did the last pop operator succeed.
   */
	operator bool() const { return did_read_; }

 private:
  size_t buffer_size() { return data.size(); }
  int NextFrom(size_t index);
  int PreviousFrom(size_t index);
  int MoveStart(int oldStart);
	int MoveEnd(int oldEnd);

  bool did_read_ = false;
  int read_index_ = 0;
  int write_index_ = 0;
  std::vector<Node> data;
};

}//namespace Sharing
}//namespace Data