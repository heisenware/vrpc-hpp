#include <boost/any.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

class Emitter {
  std::unordered_map<std::string, std::vector<boost::any>> _listeners;
  template <typename T>
  using cb_t = std::function<void(T)>;
  typedef std::function<void()> cb_void_t;

 public:
  template <typename T>
  void on(const std::string& event, const cb_t<T>& listener) {
    _listeners[event].push_back(listener);
  }

  void on(const std::string& event, const cb_void_t& listener) {
    _listeners[event].push_back(listener);
  }

  template <typename T>
  void off(const std::string& event, const cb_t<T>& listener) {
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;
    const auto& funcs = it->second;
    std::vector<boost::any> filtered;
    for (const auto& func : funcs) {
      const cb_t<T>& ref = boost::any_cast<cb_t<T>&>(func);
      if (&ref != &listener) {
        filtered.push_back(func);
      }
    }
    it->second = filtered;
  }

  void off(const std::string& event, const cb_void_t& listener) {
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;
    const auto& funcs = it->second;
    std::vector<boost::any> filtered;
    for (const auto& func : funcs) {
      const cb_void_t& ref = boost::any_cast<cb_void_t&>(func);
      if (&ref != &listener) {
        filtered.push_back(func);
      }
    }
    it->second = filtered;
  }

  void remove_all_listeners(const std::string& event) {
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;
    it->second.clear();
  }

  template <typename T>
  void emit(const std::string& event, const T& data) {
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;
    const auto& funcs = it->second;
    for (const auto& func : funcs) {
      boost::any_cast<cb_t<T>>(func)(data);
    }
  }

  void emit(const std::string& event) {
    auto it = _listeners.find(event);
    if (it == _listeners.end()) return;
    const auto& funcs = it->second;
    for (const auto& func : funcs) {
      boost::any_cast<cb_void_t>(func)();
    }
  }
};
