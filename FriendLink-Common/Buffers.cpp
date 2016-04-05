#include "Buffers.h"
#include <exception>
#include <stdexcept>
#include "Error.h"

namespace Data {
namespace Buffer {
void Circular::Push(char* carray, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    size_t bufInd = (end_ + i) % buffer_.size();
    if (bufInd == start_) {
      throw std::runtime_error("Circular Buffer is Full.");
    }
    buffer_[bufInd] = carray[i];
  }
  end_ += n;
}
std::vector<char> Circular::Pop(size_t n) {
  std::vector<char> result(n);
  for (size_t i = 0; i < n; ++i) {
    size_t bufInd = (start_ + i + 1) % buffer_.size();
    if (bufInd == end_) {
      throw std::runtime_error("Circular Buffer is Empty.");
    }
    result[i] = buffer_[bufInd];
  }
  start_ += n;
  return result;
}
size_t Circular::Length() {
  return start_ > end_ ? buffer_.size() - start_ + end_ - 1 : end_ - start_ - 1;
}
}//namespace Buffer
}//namespace Data
