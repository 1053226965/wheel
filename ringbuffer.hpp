#include <atomic>
#include <vector>

// one producer and multiple consumer
template<typename T>
class ringbuffer
{
  constexpr static int cto2n(int v) {
    v -= 1;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v += 1;
    return v;
  }

public:
  ringbuffer(int s) { _buf.resize(cto2n(s)); }

  bool put(T v) {
    size_t p = _next_can_put.load(std::memory_order_relaxed);
    if (p - _next_can_take.load(std::memory_order_relaxed) == _buf.size()) {
      return false;
    }
    _buf[calc_slot_index(p)] = v;
    _next_can_put.store(p + 1, std::memory_order_release);
    return true;
  }

  bool get(T &r) {
    size_t p = _next_can_take.load(std::memory_order_relaxed);
    
    if (_next_can_put.load(std::memory_order_acquire) <= p) {
      return false;
    }
    int v = _buf[calc_slot_index(p)];
    if (!_next_can_take.compare_exchange_strong(p, p + 1, std::memory_order_relaxed)) {
      return false;
    }
    r = v;
    return true;
  }

  size_t size() {
    return _next_can_put - _next_can_take;
  }

private:
  size_t calc_slot_index(size_t v) {
    return v & (_buf.size() - 1);
  }

private:
  constexpr static int padded_size = 64;

  std::vector<T> _buf;
  
  char _unused1[padded_size - sizeof(std::vector<T>)];

  std::atomic<size_t> _next_can_take = 0;

  char _unused2[padded_size - sizeof(std::atomic<size_t>)];

  std::atomic<size_t> _next_can_put = 0;
};
