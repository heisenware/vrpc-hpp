#include <boost/any.hpp>
#include <chrono>
#include <functional>
#include <thread>
#include <unordered_map>
#include <vector>

class Bar {
  int _value;
  std::function<void(int)> _listener;

 public:
  Bar(int value = 0) : _value(value) {}

  static int staticIncrement(int value) { return ++value; }

  static void staticCallback(const std::function<void(int)>& done, int ms) {
    std::thread([=]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      done(ms);
    }).detach();
  }

  void onValue(const std::function<void(int)>& listener) {
    _listener = listener;
  }

  int increment() {
    _value++;
    _listener(_value);
    return _value;
  }

  void reset() {
    _value = 0;
  }

  void callback(const std::function<void(int)>& done, int ms) {
    std::thread([=]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      done(_value);
    }).detach();
  }
};
