#pragma once
#include <vector>

namespace Data {
namespace Buffer {
/**
 *  A simple circular buffer of chars. That allows pushing and popping.
 */
class Circular {
public:
  /**
   *  @param buffer_size The size of the buffer.
   */
  Circular(size_t size) : buffer_(size) {}
  void Push(char*, size_t);
  /**
   *  @param buffer_size The number of characters to pop.
   */
  std::vector<char> Pop(size_t n);
  size_t Length();

private:
  std::vector<char> buffer_;
  size_t start_ = 0;
  size_t end_ = 1;
};
}//namespace Buffer
}//namespace Data