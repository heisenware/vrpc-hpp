/*
__/\\\________/\\\____/\\\\\\\\\______/\\\\\\\\\\\\\_________/\\\\\\\\\_
__\/\\\_______\/\\\__/\\\///////\\\___\/\\\/////////\\\____/\\\////////__
 __\//\\\______/\\\__\/\\\_____\/\\\___\/\\\_______\/\\\__/\\\/___________
  ___\//\\\____/\\\___\/\\\\\\\\\\\/____\/\\\\\\\\\\\\\/__/\\\_____________
   ____\//\\\__/\\\____\/\\\//////\\\____\/\\\/////////___\/\\\_____________
    _____\//\\\/\\\_____\/\\\____\//\\\___\/\\\____________\//\\\____________
     ______\//\\\\\______\/\\\_____\//\\\__\/\\\_____________\///\\\__________
      _______\//\\\_______\/\\\______\//\\\_\/\\\_______________\////\\\\\\\\\_
       ________\///________\///________\///__\///___________________\/////////__


Non-intrusively binds any C++ code and provides access in form of asynchronous
remote procedural callbacks (RPC).
Author: Dr. Burkhard C. Heisen (https://github.com/bheisen/vrpc)


Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 - 2021 Dr. Burkhard C. Heisen
<burkhard.heisen@heisenware.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef VRPC_ADAPTER_HPP
#define VRPC_ADAPTER_HPP

#define VRPC_VERSION_MAJOR 2
#define VRPC_VERSION_MINOR 6
#define VRPC_VERSION_PATCH 0

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>
#if defined(VRPC_WITH_DL) && !defined(_WIN32)
#include <dlfcn.h>
#endif

#include <vrpc/json.hpp>

#ifdef VRPC_DEBUG
#define _VRPC_DEBUG \
  if (1) std::cout << "vrpc::" << __func__ << "\t"
#else
#define _VRPC_DEBUG \
  if (0) std::cout << "vrpc::" << __func__ << "\t"
#endif

// Add std::function to json's serializable types
namespace vrpc {

template <typename R, typename... Args>
struct adl_serializer<std::function<R(Args...)>> {
  static void to_json(json& j, const std::function<R(Args...)>& func) {
    j = std::string();  // Callbacks expect a string id as argument signature
  }

  static void from_json(const json& j, std::function<R(Args...)>& func) {}
};
}  // namespace vrpc

namespace vrpc {
namespace detail {

// Reference and CV qualifier remover
template <typename T>
using no_ref_no_const =
    typename std::remove_const<typename std::remove_reference<T>::type>::type;

// Singleton helper
template <typename T>
inline T& init() {
  static T t;
  return t;
}

inline void pack_r(vrpc::json& json, char i) {}

template <class Tfirst, class... Trest>
inline void pack_r(vrpc::json& json,
                   char i,
                   const Tfirst& first,
                   const Trest&... rest) {
  char name[] = {'_', '\0', '\0', '\0'};
  name[1] = i;
  json[name] = first;
  detail::pack_r(json, i + 1, rest...);
}
}  // namespace detail

/**
 * Pack the parameters into a json object.
 * @param json Will be filled with keys _1, _2, etc. and associated values
 * @param args Any type and number of arguments forming the values
 */
template <class... Ts>
inline void pack(vrpc::json& json, const Ts&... args) {
  detail::pack_r(json, '1', args...);
}

namespace detail {

// The code below was formulated as an answer to StackOverflow and can be read
// here:
// http://stackoverflow.com/questions/10766112/c11-i-can-go-from-multiple-args-to-tuple-but-can-i-go-from-tuple-to-multiple

template <typename F, typename Tuple, bool Done, int Total, int... N>
struct call_impl {
  static auto call(F f, Tuple&& t) {
    return call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N...,
                     sizeof...(N)>::call(f, std::forward<Tuple>(t));
  }
};

template <typename F, typename Tuple, int Total, int... N>
struct call_impl<F, Tuple, true, Total, N...> {
  static auto call(F f, Tuple&& t) {
    return f(std::get<N>(std::forward<Tuple>(t))...);
  }
};
}  // namespace detail

/**
 * Call a function f with arguments unpacked from a std::tuple
 * @param f The function to be called
 * @param t A tuple containing representing the parameters
 */
template <typename F, typename Tuple>
auto call(F f, Tuple&& t) {
  typedef typename std::decay<Tuple>::type ttype;
  return detail::call_impl<
      F, Tuple, 0 == std::tuple_size<ttype>::value,
      std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
}

namespace detail {

template <std::size_t... Is>
struct indices {};

template <std::size_t N, std::size_t... Is>
struct build_indices : build_indices<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct build_indices<0, Is...> : indices<Is...> {};

template <int I>
struct placeholder {};

template <std::size_t... Is, typename Klass, typename Func>
auto variadic_bind_member(detail::indices<Is...>,
                          const Func& f,
                          const std::shared_ptr<Klass>& ptr)
    -> decltype(std::bind(f, ptr, placeholder<Is + 1>{}...)) {
  return std::bind(f, ptr, placeholder<Is + 1>{}...);
}
}  // namespace detail
}  // namespace vrpc

namespace std {
template <int I>
struct is_placeholder<vrpc::detail::placeholder<I>>
    : std::integral_constant<int, I> {};
}  // namespace std

namespace vrpc {

template <typename Klass, typename Func, typename... Args>
auto variadic_bind_member(const Func& f, const std::shared_ptr<Klass>& ptr)
    -> decltype(detail::variadic_bind_member(
        detail::build_indices<sizeof...(Args)>{},
        f,
        ptr)) {
  return detail::variadic_bind_member(detail::build_indices<sizeof...(Args)>{},
                                      f, ptr);
}

namespace detail {

template <std::size_t... Is, typename Func>
auto variadic_bind(detail::indices<Is...>, const Func& f)
    -> decltype(std::bind(f, placeholder<Is + 1>{}...)) {
  return std::bind(f, placeholder<Is + 1>{}...);
}
}  // namespace detail

template <typename Func, typename... Args>
auto variadic_bind(const Func& f)
    -> decltype(detail::variadic_bind(detail::build_indices<sizeof...(Args)>{},
                                      f)) {
  return detail::variadic_bind(detail::build_indices<sizeof...(Args)>{}, f);
}

typedef std::function<void(const vrpc::json&)> CallbackHandler;

struct Callback {
  static void register_callback_handler(const CallbackHandler& handler) {
    detail::init<CallbackHandler>() = handler;
  }
};

namespace detail {

template <typename T>
struct CallbackT;

template <typename R, typename... Args>
struct CallbackT<std::function<R(Args...)>>
    : public std::enable_shared_from_this<CallbackT<std::function<R(Args...)>>>,
      public vrpc::Callback {
  vrpc::json m_json;
  std::string m_callback_id;

  CallbackT(const vrpc::json& json, const std::string key)
      : m_json(json), m_callback_id(json["data"][key].get<std::string>()) {
    _VRPC_DEBUG << "Constructed with: " << m_json << " and " << m_callback_id
                << std::endl;
  }

  void wrapper(Args... args) {
    vrpc::json data;
    pack(data, args...);
    m_json["data"] = data;
    m_json["id"] = m_callback_id;
    _VRPC_DEBUG << "Triggering callback: " << m_callback_id
                << " with payload: " << data << std::endl;
    detail::init<CallbackHandler>()(m_json);
  }

  auto bind_wrapper() {
    auto func = &CallbackT<std::function<R(Args...)>>::wrapper;
    auto ptr = std::enable_shared_from_this<
        CallbackT<std::function<R(Args...)>>>::shared_from_this();
    return detail::variadic_bind_member(
        detail::build_indices<sizeof...(Args)>{}, func, ptr);
  }
};

template <typename... Args>
struct is_std_function : std::false_type {};

template <typename... Args>
struct is_std_function<std::function<void(Args...)>> : std::true_type {};

template <typename... Args>
struct is_std_function<const std::function<void(Args...)>&> : std::true_type {};

template <char C, typename... Args>
struct unpack_impl;

/* This specialization will remove reference and CV qualifier and bring
 * back the recursion to the standard path using vrpc::json::get().
 * Really, it should use vrpc::json::get_ref() in combination with
 * std::tie(), however currently json does not support custom types in refs.
 */
template <char C, typename A, typename... Args>
struct unpack_impl<C, A, Args...> {
  template <typename T = A,
            typename std::enable_if<std::is_reference<T>::value &&
                                        !is_std_function<T>::value,
                                    int>::type = 0>
  static auto unpack(vrpc::json& j) -> decltype(std::tuple_cat(
      std::make_tuple(std::declval<no_ref_no_const<T>>()),
      unpack_impl<C + 1, Args...>::unpack(j))) {
    // length 4 for better assembly alignment
    constexpr const char key[] = {'_', C, '\0', '\0'};
    typedef typename std::remove_const<
        typename std::remove_reference<T>::type>::type T_no_ref_no_const;
    return std::tuple_cat(
        std::make_tuple(j["data"][key].get<T_no_ref_no_const>()),
        unpack_impl<C + 1, Args...>::unpack(j));
  }

  template <typename T = A,
            typename std::enable_if<!std::is_reference<T>::value &&
                                        !is_std_function<T>::value,
                                    int>::type = 0>
  static auto unpack(vrpc::json& j)
      -> decltype(std::tuple_cat(std::make_tuple(std::declval<T>()),
                                 unpack_impl<C + 1, Args...>::unpack(j))) {
    constexpr const char key[] = {'_', C, '\0', '\0'};
    return std::tuple_cat(std::make_tuple(j["data"][key].get<T>()),
                          unpack_impl<C + 1, Args...>::unpack(j));
  }

  template <typename T = A,
            typename std::enable_if<is_std_function<T>::value, int>::type = 0>
  static auto unpack(vrpc::json& j) {
    constexpr const char key[] = {'_', C, '\0', '\0'};
    auto ptr = std::make_shared<CallbackT<no_ref_no_const<T>>>(j, key);
    return std::tuple_cat(std::make_tuple(ptr->bind_wrapper()),
                          unpack_impl<C + 1, Args...>::unpack(j));
  }
};

template <char C, typename A>
struct unpack_impl<C, A> {
  template <typename T = A,
            typename std::enable_if<std::is_reference<T>::value &&
                                        !is_std_function<T>::value,
                                    int>::type = 0>
  static std::tuple<no_ref_no_const<T>> unpack(vrpc::json& j) {
    constexpr const char key[] = {'_', C, '\0', '\0'};
    return std::make_tuple(j["data"][key].get<no_ref_no_const<T>>());
  }

  template <typename T = A,
            typename std::enable_if<!std::is_reference<T>::value &&
                                        !is_std_function<T>::value,
                                    int>::type = 0>
  static std::tuple<T> unpack(vrpc::json& j) {
    constexpr const char key[] = {'_', C, '\0', '\0'};
    return std::make_tuple(j["data"][key].get<T>());
  }

  template <typename T = A,
            typename std::enable_if<is_std_function<T>::value, int>::type = 0>
  static auto unpack(vrpc::json& j) {
    constexpr const char key[] = {'_', C, '\0', '\0'};
    auto ptr = std::make_shared<CallbackT<no_ref_no_const<T>>>(j, key);
    return std::make_tuple(ptr->bind_wrapper());
  }
};

template <char C>
struct unpack_impl<C> {
  static std::tuple<> unpack(vrpc::json&) { return std::tuple<>(); }
};
}  // namespace detail

/**
 * Unpack parameters into a tuple using perfect forwarding
 * @param j json with keys _1, _2, etc. encoding function arguments
 * @return std::tuple<Args...>
 */
template <typename... Args>
auto unpack(vrpc::json& j)
    -> decltype(detail::unpack_impl<'1', Args...>::unpack(j)) {
  return detail::unpack_impl<'1', Args...>::unpack(j);
}

namespace detail {

template <std::size_t I = 0, typename... Args>
inline typename std::enable_if<I == sizeof...(Args), void>::type get_signature(
    const std::string& signature,
    const std::tuple<Args...>&) {
  // end recursion
}

template <std::size_t I = 0, typename... Args>
    inline typename std::enable_if <
    I<sizeof...(Args), void>::type get_signature(std::string& signature,
                                                 const std::tuple<Args...>& t) {
  vrpc::json j{{"t", std::get<I>(t)}};
  signature += j["t"].type_name();
  return get_signature<I + 1, Args...>(signature, t);
}
}  // namespace detail

template <typename... Args>
inline std::string get_signature() {
  std::string signature;
  detail::get_signature(
      signature, std::tuple<typename std::remove_reference<Args>::type...>());
  return signature.empty() ? signature : "-" + signature;
}

inline std::string get_signature(const vrpc::json& json) {
  std::string signature;
  for (const auto& it : json) {
    signature += it.type_name();
  }
  return signature.empty() ? signature : "-" + signature;
}

namespace detail {

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
}  // namespace detail

// Value class
class Value {
 public:
  Value() noexcept : content(0) {}

  template <typename T>
  Value(const T& value)
      : content(new holder<
                typename std::remove_cv<typename std::decay<T>::type>::type>(
            value)) {}

  Value(const char* const& value) : Value(std::string(value)) {}

  /// Copy constructor
  Value(const Value& other)
      : content(other.content ? other.content->clone() : 0) {}

  /// Move constructor
  Value(Value&& other) : content(other.content) { other.content = 0; }

  template <typename T>
  Value(T&& value,
        typename std::enable_if<!std::is_same<Value&, T>::value>::type* = 0,
        typename std::enable_if<!std::is_const<T>::value>::type* = 0,
        typename std::enable_if<!detail::is_shared_ptr<T>::value>::type* = 0)
      : content(new holder<
                typename std::remove_cv<typename std::decay<T>::type>::type>(
            std::forward<T>(value))) {}

  ~Value() noexcept { delete content; }

 public:  // functions
  Value& swap(Value& rhs) noexcept {
    std::swap(content, rhs.content);
    return *this;
  }

  Value& operator=(const Value& rhs) {
    Value(rhs).swap(*this);
    return *this;
  }

  Value& operator=(Value&& rhs) noexcept {
    rhs.swap(*this);
    Value().swap(rhs);
    return *this;
  }

  template <typename T>
  Value& operator=(T&& rhs) {
    Value(static_cast<T&&>(rhs)).swap(*this);
    return *this;
  }

  Value& operator=(const char* const& rhs) {
    Value(std::string(rhs)).swap(*this);
    return *this;
  }

  // NOTE: Windows compiler is broken here
  // template <typename T>
  // operator const T&() const {
  //   return const_cast<Value*> (this)->template get<T>();
  // }

  // template <typename T>
  // operator T() const {
  //   return const_cast<Value*> (this)->template get<T>();
  // }

  template <typename T>
  T& operator=(T& rhs) {
    Value(rhs).swap(*this);
    return *this;
  }

  bool empty() const noexcept { return !content; }

  void clear() noexcept { Value().swap(*this); }

  std::type_index type() const noexcept {
    static std::type_index void_type(typeid(void));
    return content ? content->type() : void_type;
  }

  template <typename T>
  inline const T& get() const {
    // TODO Check type correctness here
    // TODO implement by specialization and not by overload
    return _get<T>(typename detail::is_shared_ptr<T>::type());
  }

 private:  // functions
  template <typename T>
  inline const T& _get(const std::true_type&) const {
    return static_cast<holder<
        typename std::remove_cv<typename T::element_type>::type>*>(content)
        ->held;
  }

  template <typename T>
  inline const T& _get(const std::false_type&) const {
    return *(
        static_cast<holder<typename std::remove_cv<T>::type>*>(content)->held);
  }

 private:  // types
  class placeholder {
   public:  // structors
    virtual ~placeholder() {}

   public:  // queries
    virtual std::type_index type() const noexcept = 0;

    virtual placeholder* clone() const = 0;
  };

  template <typename T>
  class holder : public placeholder {
   public:  // structors
    holder(const T& value) : held(new T(value)) {}

    holder(T&& value) : held(new T(static_cast<T&&>(value))) {}

   public:  // queries
    virtual std::type_index type() const noexcept {
      return std::type_index(typeid(T));
    }

    virtual placeholder* clone() const {
      // This copies a std::shared_ptr!
      return new holder(*held);
    }

    holder& operator=(const holder&) = delete;

   public:  // representation
    std::shared_ptr<T> held;
  };

  template <typename T>
  class holder<std::shared_ptr<T>> : public placeholder {
   public:
    holder(const std::shared_ptr<T>& value) : held(value) {}

   public:  // queries
    virtual std::type_index type() const noexcept {
      return std::type_index(typeid(T));
    }

    virtual placeholder* clone() const {
      // This copies a std::shared_ptr!
      return new holder(held);
    }

    holder& operator=(const holder&) = delete;

   public:  // representation
    std::shared_ptr<T> held;
  };

  placeholder* content;
};

inline bool operator==(const Value& lhs, const char& rhs) {
  return lhs.get<char>() == rhs;
}

class Function {
 public:
  Function() {}

  virtual ~Function() {}

  template <typename T>
  void bind_instance(const std::shared_ptr<T>& instance) {
    this->do_bind_instance(Value(instance));
  }

  void call_function(vrpc::json& json) { this->do_call_function(json); }

  std::shared_ptr<Function> clone() { return this->do_clone(); }

 protected:
  virtual void do_bind_instance(const Value& instance) = 0;
  virtual void do_call_function(vrpc::json& json) = 0;
  virtual std::shared_ptr<Function> do_clone() = 0;
};

template <typename Klass, typename Lambda, typename Ret, typename... Args>
class MemberFunction : public Function {
  Lambda m_lambda;
  std::shared_ptr<Klass> m_ptr;

 public:
  MemberFunction(const Lambda& lambda) : m_lambda(lambda) {}

  virtual ~MemberFunction() = default;

  virtual std::shared_ptr<Function> do_clone() {
    auto ptr = std::make_shared<MemberFunction>(m_lambda);
    return std::static_pointer_cast<Function>(ptr);
  }

  virtual void do_call_function(vrpc::json& json) {
    try {
      json["data"]["r"] =
          vrpc::call(m_lambda(m_ptr), vrpc::unpack<Args...>(json));
    } catch (const std::exception& e) {
      json["data"]["e"] = std::string(e.what());
    }
  }

  virtual void do_bind_instance(const Value& instance) {
    m_ptr = instance.get<std::shared_ptr<Klass>>();
  }
};

template <typename Klass, typename Lambda, typename... Args>
class MemberFunction<Klass, Lambda, void, Args...> : public Function {
  Lambda m_lambda;
  std::shared_ptr<Klass> m_ptr;

 public:
  MemberFunction(const Lambda& lambda) : m_lambda(lambda) {}

  virtual ~MemberFunction() = default;

  virtual std::shared_ptr<Function> do_clone() {
    auto ptr = std::make_shared<MemberFunction>(m_lambda);
    return std::static_pointer_cast<Function>(ptr);
  }

  virtual void do_call_function(vrpc::json& json) {
    try {
      vrpc::call(m_lambda(m_ptr), vrpc::unpack<Args...>(json));
      json["data"]["r"] = nullptr;
    } catch (const std::exception& e) {
      json["data"]["e"] = std::string(e.what());
    }
  }

  virtual void do_bind_instance(const Value& instance) {
    m_ptr = instance.get<std::shared_ptr<Klass>>();
  }
};

template <typename Lambda, typename Ret, typename... Args>
class StaticFunction : public Function {
  Lambda m_lambda;

 public:
  StaticFunction(const Lambda& lambda) : m_lambda(lambda) {}

  virtual ~StaticFunction() = default;

  virtual std::shared_ptr<Function> do_clone() {
    auto ptr = std::make_shared<StaticFunction>(m_lambda);
    return std::static_pointer_cast<Function>(ptr);
  }

  virtual void do_call_function(vrpc::json& json) {
    try {
      json["data"]["r"] = vrpc::call(m_lambda(), vrpc::unpack<Args...>(json));
    } catch (const std::exception& e) {
      json["data"]["e"] = std::string(e.what());
    }
  }

  virtual void do_bind_instance(const Value&) {
    // Nothing to bind, static function
  }
};

template <typename Lambda, typename... Args>
class StaticFunction<Lambda, void, Args...> : public Function {
  Lambda m_lambda;

 public:
  StaticFunction(const Lambda& lambda) : m_lambda(lambda) {}

  virtual ~StaticFunction() = default;

  virtual std::shared_ptr<Function> do_clone() {
    auto ptr = std::make_shared<StaticFunction>(m_lambda);
    return std::static_pointer_cast<Function>(ptr);
  }

  virtual void do_call_function(vrpc::json& json) {
    try {
      vrpc::call(m_lambda(), vrpc::unpack<Args...>(json));
      json["data"]["r"] = nullptr;
    } catch (const std::exception& e) {
      json["data"]["e"] = std::string(e.what());
    }
  }

  virtual void do_bind_instance(const Value&) {
    // Nothing to bind, static function
  }
};

template <typename Lambda, typename... Args>
class ConstructorFunction : public Function {
  Lambda m_lambda;

 public:
  ConstructorFunction(const Lambda& lambda) : m_lambda(lambda) {}

  virtual ~ConstructorFunction() = default;

  virtual std::shared_ptr<Function> do_clone() {
    auto ptr = std::make_shared<ConstructorFunction>(m_lambda);
    return std::static_pointer_cast<Function>(ptr);
  }

  virtual void do_call_function(vrpc::json& json) {
    try {
      json["data"]["r"] = vrpc::call(m_lambda, vrpc::unpack<Args...>(json));
    } catch (const std::exception& e) {
      json["data"]["e"] = std::string(e.what());
    }
  }

  virtual void do_bind_instance(const Value&) {
    // Nothing to bind
  }
};

struct required {};

namespace detail {
struct param {
  json j;
  template <typename DefaultType>
  param(const std::string& name,
        const DefaultType& default_value,
        const std::string& description) {
    // json tmp{{"t", type()}};
    // j["type"] = tmp["t"].type_name();
    j["name"] = name;
    j["optional"] = true;
    j["default"] = default_value;
    j["description"] = description;
  }
  param(const std::string& name,
        const required& default_value,
        const std::string& description) {
    // json tmp{{"t", type()}};
    // j["type"] = tmp["t"].type_name();
    j["name"] = name;
    j["optional"] = false;
    j["default"] = nullptr;
    j["description"] = description;
  }
};

template <typename ValueType>
struct ret {
  json j;
  ret(const std::string& description) {
    j["type"] = json(ValueType()).type_name();
    j["description"] = description;
  }
};
template <>
struct ret<void> {
  json j;
  ret(const std::string& description) {
    j["type"] = "void";
    j["description"] = description;
  }
};
}  // namespace detail

class LocalFactory {
  friend LocalFactory& detail::init<LocalFactory>();
  friend class Proxy;
  friend class MqttClient;

  typedef std::unordered_map<std::string, Value> StringAnyMap;
  typedef std::unordered_map<std::string, std::shared_ptr<Function>>
      StringFunctionMap;
  typedef std::unordered_map<std::string, StringFunctionMap> FunctionRegistry;
  typedef std::unordered_map<std::string, std::string> NamedInstances;
  typedef std::unordered_map<std::string, vrpc::json> MetaData;

  // Maps: class_name => function_name => functionCallback
  FunctionRegistry m_class_function_registry;
  // Maps: instanceId => function_name => functionCallback
  FunctionRegistry m_function_registry;
  // Maps: instanceId => instanceObj
  StringAnyMap m_instances;
  // Maps: instanceId => class_name
  NamedInstances m_named_instances;
  // Optional schema information
  MetaData m_meta_data;

 public:
  template <typename Klass, typename... Args>
  static void register_constructor(const std::string& class_name) {
    LocalFactory::inject_create_function<Klass, Args...>(class_name);
    LocalFactory::inject_create_named_function<Klass, Args...>(class_name);
    LocalFactory::inject_get_named_function<Klass>(class_name);
    LocalFactory::inject_delete_function<Klass>(class_name);
  }

  template <typename Klass,
            typename Func,
            Func f,
            typename Ret,
            typename... Args>
  static void register_member_function(const std::string& class_name,
                                       const std::string& function_name) {
    auto func = [](const std::shared_ptr<Klass>& ptr) {
      return vrpc::variadic_bind_member<Klass, Func, Args...>(f, ptr);
    };
    auto funcT =
        std::make_shared<MemberFunction<Klass, decltype(func), Ret, Args...>>(
            func);
    detail::init<LocalFactory>()
        .m_class_function_registry[class_name][function_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << function_name
                << std::endl;
  }

  template <typename Func, Func f, typename Ret, typename... Args>
  static void register_static_function(const std::string& class_name,
                                       const std::string& function_name) {
    auto func = []() { return vrpc::variadic_bind<Func, Args...>(f); };
    auto funcT =
        std::make_shared<StaticFunction<decltype(func), Ret, Args...>>(func);
    detail::init<LocalFactory>()
        .m_function_registry[class_name][function_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << function_name
                << std::endl;
  }

  static void register_meta_data(const std::string& class_name,
                                 const std::string& function_name,
                                 const std::string& description,
                                 const json& params,
                                 const json& ret) {
    LocalFactory& rf = detail::init<LocalFactory>();
    vrpc::json& j = rf.m_meta_data[class_name];
    j[function_name]["description"] = description;
    j[function_name]["params"] = params;
    j[function_name]["ret"] = ret;
  }

  static std::vector<std::string> get_instances(const std::string& class_name) {
    std::vector<std::string> instances;
    for (const auto& kv : detail::init<LocalFactory>().m_named_instances) {
      if (kv.second == class_name) instances.push_back(kv.first);
    }
    return instances;
  }

  static std::vector<std::string> get_member_functions(
      const std::string& class_name) {
    std::vector<std::string> functions;
    const auto& it =
        detail::init<LocalFactory>().m_class_function_registry.find(class_name);
    if (it != detail::init<LocalFactory>().m_class_function_registry.end()) {
      for (const auto& kv : it->second) {
        functions.push_back(kv.first);
      }
    }
    return functions;
  }

  static std::vector<std::string> get_static_functions(
      const std::string& class_name) {
    std::vector<std::string> functions;
    const auto& it =
        detail::init<LocalFactory>().m_function_registry.find(class_name);
    if (it != detail::init<LocalFactory>().m_function_registry.end()) {
      for (const auto& kv : it->second) {
        functions.push_back(kv.first);
      }
    }
    return functions;
  }

  static std::vector<std::string> get_classes() {
    std::vector<std::string> classes;
    for (const auto& kv :
         detail::init<LocalFactory>().m_class_function_registry) {
      classes.push_back(kv.first);
    }
    return classes;
  }

  static json get_meta_data(const std::string& class_name) {
    LocalFactory& rf = detail::init<LocalFactory>();
    return rf.m_meta_data[class_name];
  }

  static std::string call(const std::string& jsonString) {
    vrpc::json json;
    json = vrpc::json::parse(jsonString);
    LocalFactory::call(json);
    return json.dump();
  }

  static void call(vrpc::json& json) {
    const std::string& context = json["context"];
    std::string function = json["method"];
    vrpc::json& args = json["data"];
    function += vrpc::get_signature(args);
    _VRPC_DEBUG << "Calling function: " << function << " with payload: " << args
                << std::endl;
    auto it_t = detail::init<LocalFactory>().m_function_registry.find(context);
    if (it_t != detail::init<LocalFactory>().m_function_registry.end()) {
      auto it_f = it_t->second.find(function);
      if (it_f != it_t->second.end()) {
        it_f->second->call_function(json);
        return;
      }
      json["data"]["e"] = "Could not find function: " + function;
      return;
    }
    json["data"]["e"] = "Could not find context: " + context;
  }

  static void load_bindings(const std::string& path) {
#if defined(VRPC_WITH_DL) && !defined(_WIN32)
    void* libHandle = dlopen(path.c_str(), RTLD_LAZY);
    if (libHandle == 0) {
      throw std::runtime_error("Problem loading bindings: " +
                               std::string(dlerror()));
    }
#else
    _VRPC_DEBUG << "Ignored call, dynamic loading is disabled" << std::endl;
#endif
  }

 private:
  LocalFactory() {}

  LocalFactory(const LocalFactory&) = default;

  virtual ~LocalFactory() = default;

  template <typename Klass>
  static std::string create_instance_id(
      const std::shared_ptr<Klass>& ptr) noexcept {
    std::ostringstream ss;
    ss << reinterpret_cast<std::uintptr_t>(ptr.get());
    return ss.str();
  }

  template <typename Klass, typename... Args>
  static void inject_create_function(const std::string& class_name) {
    auto func = [=](Args... args) {
      LocalFactory& rf = detail::init<LocalFactory>();
      // Create instance
      auto ptr = std::shared_ptr<Klass>(new Klass(args...));
      // Generate instance_id
      const std::string instance_id = create_instance_id(ptr);
      // Bind member functions
      for (auto& i : rf.m_class_function_registry[class_name]) {
        auto functionCallback = i.second->clone();
        functionCallback->bind_instance(ptr);
        rf.m_function_registry[instance_id][i.first] = functionCallback;
      }
      // Keep instance alive by saving the shared_ptr
      rf.m_instances[instance_id] = Value(ptr);
      return instance_id;
    };
    auto funcT =
        std::make_shared<ConstructorFunction<decltype(func), Args...>>(func);
    const std::string func_name("__create__" + vrpc::get_signature<Args...>());
    detail::init<LocalFactory>().m_function_registry[class_name][func_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << func_name
                << std::endl;
  }

  template <typename Klass, typename... Args>
  static void inject_create_named_function(const std::string& class_name) {
    auto func = [=](const std::string& instance_id, Args... args) {
      LocalFactory& rf = detail::init<LocalFactory>();
      const auto& it = rf.m_instances.find(instance_id);
      if (it != rf.m_instances.end()) return instance_id;
      // Create instance
      auto ptr = std::shared_ptr<Klass>(new Klass(args...));
      // Bind member functions
      for (auto& i : rf.m_class_function_registry[class_name]) {
        auto functionCallback = i.second->clone();
        functionCallback->bind_instance(ptr);
        rf.m_function_registry[instance_id][i.first] = functionCallback;
      }
      // Keep instance alive by saving the shared_ptr
      rf.m_instances[instance_id] = Value(ptr);
      // Store named instance
      rf.m_named_instances.insert({instance_id, class_name});
      return instance_id;
    };
    auto funcT = std::make_shared<
        ConstructorFunction<decltype(func), const std::string&, Args...>>(func);
    const std::string func_name("__createNamed__" +
                                vrpc::get_signature<std::string, Args...>());
    detail::init<LocalFactory>().m_function_registry[class_name][func_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << func_name
                << std::endl;
  }

  template <typename Klass>
  static void inject_get_named_function(const std::string& class_name) {
    auto func = [=](const std::string& instance_id) {
      LocalFactory& rf = detail::init<LocalFactory>();
      const auto& it = rf.m_instances.find(instance_id);
      if (it != rf.m_instances.end()) return instance_id;
      throw std::runtime_error("Instance with id: " + instance_id +
                               " does not exist");
    };
    auto funcT = std::make_shared<
        ConstructorFunction<decltype(func), const std::string&>>(func);
    const std::string func_name("__getNamed__" +
                                vrpc::get_signature<std::string>());
    detail::init<LocalFactory>().m_function_registry[class_name][func_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << func_name
                << std::endl;
  }

  template <typename Klass>
  static void inject_delete_function(const std::string& class_name) {
    auto func = [=](const std::string& instance_id) {
      LocalFactory& rf = detail::init<LocalFactory>();
      const auto& it = rf.m_instances.find(instance_id);
      if (it == rf.m_instances.end()) return false;
      rf.m_function_registry.erase(instance_id);
      rf.m_instances.erase(instance_id);
      rf.m_named_instances.erase(instance_id);
      return true;
    };
    auto funcT = std::make_shared<
        ConstructorFunction<decltype(func), const std::string&>>(func);
    const std::string func_name("__delete__" +
                                vrpc::get_signature<std::string>());
    detail::init<LocalFactory>().m_function_registry[class_name][func_name] =
        std::static_pointer_cast<Function>(funcT);
    _VRPC_DEBUG << "Registered: " << class_name << "::" << func_name
                << std::endl;
  }
};

namespace detail {

template <class Klass, typename... Args>
struct CtorRegistrar {
  CtorRegistrar(const std::string& class_name) {
    LocalFactory::register_constructor<Klass, Args...>(class_name);
  }
};

template <class Klass, typename... Args>
struct RegisterCtor {
  static const CtorRegistrar<Klass, Args...> registerAs;
};

template <class Klass, typename... Args>
struct CtorXRegistrar {
  CtorXRegistrar(const std::string& class_name,
                 const std::string& ctor_description,
                 const ret<std::string>& ret,
                 const std::vector<param> params = {}) {
    LocalFactory::register_constructor<Klass, Args...>(class_name);
    const std::string ctor_name("__createNamed__" +
                                vrpc::get_signature<std::string, Args...>());
    json jp;
    for (const auto& param : params) {
      jp.push_back(param.j);
    }
    LocalFactory::register_meta_data(class_name, ctor_name, ctor_description,
                                     jp, ret.j);
  }
};

template <class Klass, typename... Args>
struct RegisterCtorX {
  static const CtorXRegistrar<Klass, Args...> registerAs;
};

template <class Klass, typename Func, Func f, typename Ret, typename... Args>
struct MemberFunctionRegistrar {
  MemberFunctionRegistrar(const std::string& class_name,
                          const std::string& function_name) {
    const std::string full_function_name(function_name +
                                         vrpc::get_signature<Args...>());
    LocalFactory::register_member_function<Klass, Func, f, Ret, Args...>(
        class_name, full_function_name);
  }
};

template <class Klass, typename Func, Func f, typename Ret, typename... Args>
struct MemberFunctionXRegistrar {
  MemberFunctionXRegistrar(const std::string& class_name,
                           const std::string& function_name,
                           const std::string& function_description,
                           const ret<Ret>& ret,
                           const std::vector<param> params = {}) {
    const std::string full_function_name(function_name +
                                         vrpc::get_signature<Args...>());
    LocalFactory::register_member_function<Klass, Func, f, Ret, Args...>(
        class_name, full_function_name);
    json jp;
    for (const auto& param : params) {
      jp.push_back(param.j);
    }
    LocalFactory::register_meta_data(class_name, full_function_name,
                                     function_description, jp, ret.j);
  }
};

template <class Klass, typename Func, Func f, typename Ret, typename... Args>
struct RegisterMemberFunction {
  static const MemberFunctionRegistrar<Klass, Func, f, Ret, Args...> registerAs;
};

template <class Klass, typename Func, Func f, typename Ret, typename... Args>
struct RegisterMemberFunctionX {
  static const MemberFunctionXRegistrar<Klass, Func, f, Ret, Args...>
      registerAs;
};

template <typename Func, Func f, typename Ret, typename... Args>
struct StaticFunctionRegistrar {
  StaticFunctionRegistrar(const std::string& class_name,
                          const std::string& function_name) {
    const std::string full_function_name(function_name +
                                         vrpc::get_signature<Args...>());
    LocalFactory::register_static_function<Func, f, Ret, Args...>(
        class_name, full_function_name);
  }
};

template <typename Func, Func f, typename Ret, typename... Args>
struct RegisterStaticFunction {
  static const StaticFunctionRegistrar<Func, f, Ret, Args...> registerAs;
};

template <typename Func, Func f, typename Ret, typename... Args>
struct StaticFunctionXRegistrar {
  StaticFunctionXRegistrar(const std::string& class_name,
                           const std::string& function_name,
                           const std::string& function_description,
                           const ret<Ret>& ret,
                           const std::vector<param> params = {}) {
    const std::string full_function_name(function_name +
                                         vrpc::get_signature<Args...>());
    LocalFactory::register_static_function<Func, f, Ret, Args...>(
        class_name, full_function_name);
    json jp;
    for (const auto& param : params) {
      jp.push_back(param.j);
    }
    LocalFactory::register_meta_data(class_name, full_function_name,
                                     function_description, jp, ret.j);
  }
};

template <typename Func, Func f, typename Ret, typename... Args>
struct RegisterStaticFunctionX {
  static const StaticFunctionXRegistrar<Func, f, Ret, Args...> registerAs;
};
}  // namespace detail

// ####################### Macro utility #######################

#define CAT(A, B) A##B
#define SELECT(NAME, NUM) CAT(_##NAME##_, NUM)
#define GET_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, \
                  _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,  \
                  _27, _28, _29, _30 /* ad nauseam */, COUNT, ...)             \
  COUNT
#define VA_SIZE(...)                                                         \
  GET_COUNT(__VA_ARGS__, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, \
            17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define VA_SELECT(NAME, ...) SELECT(NAME, VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

#define VRPC_CTOR(...) VA_SELECT(VRPC_CTOR, __VA_ARGS__)
#define VRPC_MEMBER_FUNCTION(...) VA_SELECT(VRPC_MEMBER_FUNCTION, __VA_ARGS__)
#define VRPC_CONST_MEMBER_FUNCTION(...) \
  VA_SELECT(VRPC_CONST_MEMBER_FUNCTION, __VA_ARGS__)
#define VRPC_STATIC_FUNCTION(...) VA_SELECT(VRPC_STATIC_FUNCTION, __VA_ARGS__)

#define VRPC_CTOR_X(...) VA_SELECT(VRPC_CTOR_X, __VA_ARGS__)
#define VRPC_MEMBER_FUNCTION_X(...) \
  VA_SELECT(VRPC_MEMBER_FUNCTION_X, __VA_ARGS__)
#define VRPC_STATIC_FUNCTION_X(...) \
  VA_SELECT(VRPC_STATIC_FUNCTION_X, __VA_ARGS__)

// deprecate
#define VRPC_MEMBER_FUNCTION_CONST(...) \
  VA_SELECT(VRPC_CONST_MEMBER_FUNCTION, __VA_ARGS__)
#define VRPC_VOID_MEMBER_FUNCTION(...) \
  VA_SELECT(VRPC_VOID_MEMBER_FUNCTION, __VA_ARGS__)
#define VRPC_VOID_MEMBER_FUNCTION_CONST(...) \
  VA_SELECT(VRPC_VOID_MEMBER_FUNCTION_CONST, __VA_ARGS__)
#define VRPC_VOID_STATIC_FUNCTION(...) \
  VA_SELECT(VRPC_VOID_STATIC_FUNCTION, __VA_ARGS__)

//  ####################### Callbacks #######################

#define VRPC_CALLBACK(...) const std::function<void(__VA_ARGS__)>&

//  ####################### Constructors #######################

#define _VRPC_CTOR_RET_DESC "returns the id of the created instance"

#define _VRPC_CTOR_VA_ARGS(Klass, ...)                  \
  template <>                                           \
  const vrpc::detail::CtorRegistrar<Klass, __VA_ARGS__> \
      vrpc::detail::RegisterCtor<Klass, __VA_ARGS__>::registerAs(#Klass);

#define _VRPC_CTOR_1(Klass)                \
  template <>                              \
  const vrpc::detail::CtorRegistrar<Klass> \
      vrpc::detail::RegisterCtor<Klass>::registerAs(#Klass);

#define _VRPC_CTOR_2(Klass, A1) _VRPC_CTOR_VA_ARGS(Klass, A1)
#define _VRPC_CTOR_3(Klass, A1, A2) _VRPC_CTOR_VA_ARGS(Klass, A1, A2)
#define _VRPC_CTOR_4(Klass, A1, A2, A3) _VRPC_CTOR_VA_ARGS(Klass, A1, A2, A3)
#define _VRPC_CTOR_5(Klass, A1, A2, A3, A4) \
  _VRPC_CTOR_VA_ARGS(Klass, A1, A2, A3, A4)
#define _VRPC_CTOR_6(Klass, A1, A2, A3, A4, A5) \
  _VRPC_CTOR_VA_ARGS(Klass, A1, A2, A3, A4, A5)
#define _VRPC_CTOR_7(Klass, A1, A2, A3, A4, A5, A6) \
  _VRPC_CTOR_VA_ARGS(Klass, A1, A2, A3, A4, A5, A6)

#define _VRPC_CTOR_X_2(Klass, CtorDesc)                                \
  template <>                                                          \
  const vrpc::detail::CtorXRegistrar<Klass>                            \
      vrpc::detail::RegisterCtorX<Klass>::registerAs(#Klass, CtorDesc, \
                                                     {_VRPC_CTOR_RET_DESC});

#define _VRPC_CTOR_X_6(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc) \
  template <>                                                          \
  const vrpc::detail::CtorXRegistrar<Klass, A1>                        \
      vrpc::detail::RegisterCtorX<Klass, A1>::registerAs(              \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                     \
          {{A1Name, A1Default, A1Desc}});

#define _VRPC_CTOR_X_10(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc, A2, \
                        A2Name, A2Default, A2Desc)                          \
  template <>                                                               \
  const vrpc::detail::CtorXRegistrar<Klass, A1, A2>                         \
      vrpc::detail::RegisterCtorX<Klass, A1, A2>::registerAs(               \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                          \
          {{A1Name, A1Default, A1Desc}, {A2Name, A2Default, A2Desc}});

#define _VRPC_CTOR_X_14(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc, A2, \
                        A2Name, A2Default, A2Desc, A3, A3Name, A3Default,   \
                        A3Desc)                                             \
  template <>                                                               \
  const vrpc::detail::CtorXRegistrar<Klass, A1, A2, A3>                     \
      vrpc::detail::RegisterCtorX<Klass, A1, A2, A3>::registerAs(           \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                          \
          {{A1Name, A1Default, A1Desc},                                     \
           {A2Name, A2Default, A2Desc},                                     \
           {A3Name, A3Default, A3Desc}});

#define _VRPC_CTOR_X_18(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc, A2, \
                        A2Name, A2Default, A2Desc, A3, A3Name, A3Default,   \
                        A3Desc, A4, A4Name, A4Default, A4Desc)              \
  template <>                                                               \
  const vrpc::detail::CtorXRegistrar<Klass, A1, A2, A3, A4>                 \
      vrpc::detail::RegisterCtorX<Klass, A1, A2, A3, A4>::registerAs(       \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                          \
          {{A1Name, A1Default, A1Desc},                                     \
           {A2Name, A2Default, A2Desc},                                     \
           {A3Name, A3Default, A3Desc},                                     \
           {A4Name, A4Default, A4Desc}});

#define _VRPC_CTOR_X_22(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc, A2, \
                        A2Name, A2Default, A2Desc, A3, A3Name, A3Default,   \
                        A3Desc, A4, A4Name, A4Default, A4Desc, A5, A5Name,  \
                        A5Default, A5Desc)                                  \
  template <>                                                               \
  const vrpc::detail::CtorXRegistrar<Klass, A1, A2, A3, A4, A5>             \
      vrpc::detail::RegisterCtorX<Klass, A1, A2, A3, A4, A5>::registerAs(   \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                          \
          {{A1Name, A1Default, A1Desc},                                     \
           {A2Name, A2Default, A2Desc},                                     \
           {A3Name, A3Default, A3Desc},                                     \
           {A4Name, A4Default, A4Desc},                                     \
           {A5Name, A5Default, A5Desc}});

#define _VRPC_CTOR_X_26(Klass, CtorDesc, A1, A1Name, A1Default, A1Desc, A2,   \
                        A2Name, A2Default, A2Desc, A3, A3Name, A3Default,     \
                        A3Desc, A4, A4Name, A4Default, A4Desc, A5, A5Name,    \
                        A5Default, A5Desc, A6, A6Name, A6Default, A6Desc)     \
  template <>                                                                 \
  const vrpc::detail::CtorXRegistrar<Klass, A1, A2, A3, A4, A5, A6>           \
      vrpc::detail::RegisterCtorX<Klass, A1, A2, A3, A4, A5, A6>::registerAs( \
          #Klass, CtorDesc, {_VRPC_CTOR_RET_DESC},                            \
          {{A1Name, A1Default, A1Desc},                                       \
           {A2Name, A2Default, A2Desc},                                       \
           {A3Name, A3Default, A3Desc},                                       \
           {A4Name, A4Default, A4Desc},                                       \
           {A5Name, A5Default, A5Desc},                                       \
           {A6Name, A6Default, A6Desc}});

// deprecate
#define VRPC_VOID_CTOR(Klass)              \
  template <>                              \
  const vrpc::detail::CtorRegistrar<Klass> \
      vrpc::detail::RegisterCtor<Klass>::registerAs(#Klass);

// ####################### Zero arguments #######################

#define _VRPC_MEMBER_FUNCTION_3(Klass, Ret, Function)                       \
  template <>                                                               \
  const vrpc::detail::MemberFunctionRegistrar<                              \
      Klass, decltype(static_cast<Ret (Klass::*)()>(&Klass::Function)),     \
      &Klass::Function, Ret>                                                \
      vrpc::detail::RegisterMemberFunction<                                 \
          Klass, decltype(static_cast<Ret (Klass::*)()>(&Klass::Function)), \
          &Klass::Function, Ret>::registerAs(#Klass, #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_3(Klass, Ret, Function)                   \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionRegistrar<                                \
      Klass, decltype(static_cast<Ret (Klass::*)() const>(&Klass::Function)), \
      &Klass::Function, Ret>                                                  \
      vrpc::detail::RegisterMemberFunction<                                   \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)() const>(&Klass::Function)),    \
          &Klass::Function, Ret>::registerAs(#Klass, #Function);

#define _VRPC_STATIC_FUNCTION_3(Klass, Ret, Function)                          \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionRegistrar<decltype(static_cast<Ret (*)()>( \
                                                  Klass::Function)),           \
                                              &Klass::Function, Ret>           \
      vrpc::detail::RegisterStaticFunction<                                    \
          decltype(static_cast<Ret (*)()>(Klass::Function)), &Klass::Function, \
          Ret>::registerAs(#Klass, #Function);

#define _VRPC_MEMBER_FUNCTION_X_5(Klass, Ret, RetDesc, Function, FunctionDesc) \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionXRegistrar<                                \
      Klass, decltype(static_cast<Ret (Klass::*)()>(&Klass::Function)),        \
      &Klass::Function, Ret>                                                   \
      vrpc::detail::RegisterMemberFunctionX<                                   \
          Klass, decltype(static_cast<Ret (Klass::*)()>(&Klass::Function)),    \
          &Klass::Function, Ret>::registerAs(#Klass, #Function, FunctionDesc,  \
                                             {RetDesc});

#define _VRPC_CONST_MEMBER_FUNCTION_X_5(Klass, Ret, RetDesc, Function,        \
                                        FunctionDesc)                         \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass, decltype(static_cast<Ret (Klass::*)() const>(&Klass::Function)), \
      &Klass::Function, Ret>                                                  \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)() const>(&Klass::Function)),    \
          &Klass::Function, Ret>::registerAs(#Klass, #Function, FunctionDesc, \
                                             {RetDesc});

#define _VRPC_STATIC_FUNCTION_X_5(Klass, Ret, RetDesc, Function, FunctionDesc) \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionXRegistrar<                                \
      decltype(static_cast<Ret (*)()>(Klass::Function)), &Klass::Function,     \
      Ret>                                                                     \
      vrpc::detail::RegisterStaticFunctionX<                                   \
          decltype(static_cast<Ret (*)()>(Klass::Function)), &Klass::Function, \
          Ret>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc});

#define _VRPC_VOID_MEMBER_FUNCTION_2(Klass, Function) \
  _VRPC_MEMBER_FUNCTION_3(Klass, void, Function)

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_2(Klass, Function) \
  _VRPC_CONST_MEMBER_FUNCTION_3(Klass, void, Function)

#define _VRPC_VOID_STATIC_FUNCTION_2(Klass, Function) \
  _VRPC_STATIC_FUNCTION_3(Klass, void, Function)

// ####################### One argument #######################

#define _VRPC_MEMBER_FUNCTION_4(Klass, Ret, Function, A1)                     \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionRegistrar<                                \
      Klass, decltype(static_cast<Ret (Klass::*)(A1)>(&Klass::Function)),     \
      &Klass::Function, Ret, A1>                                              \
      vrpc::detail::RegisterMemberFunction<                                   \
          Klass, decltype(static_cast<Ret (Klass::*)(A1)>(&Klass::Function)), \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_4(Klass, Ret, Function, A1)              \
  template <>                                                                \
  const vrpc::detail::MemberFunctionRegistrar<                               \
      Klass,                                                                 \
      decltype(static_cast<Ret (Klass::*)(A1) const>(&Klass::Function)),     \
      &Klass::Function, Ret, A1>                                             \
      vrpc::detail::RegisterMemberFunction<                                  \
          Klass,                                                             \
          decltype(static_cast<Ret (Klass::*)(A1) const>(&Klass::Function)), \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function);

#define _VRPC_STATIC_FUNCTION_4(Klass, Ret, Function, A1)                    \
  template <>                                                                \
  const vrpc::detail::StaticFunctionRegistrar<                               \
      decltype(static_cast<Ret (*)(A1)>(Klass::Function)), &Klass::Function, \
      Ret, A1>                                                               \
      vrpc::detail::RegisterStaticFunction<                                  \
          decltype(static_cast<Ret (*)(A1)>(Klass::Function)),               \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function);

#define _VRPC_MEMBER_FUNCTION_X_9(Klass, Ret, RetDesc, Function, FunctionDesc, \
                                  A1, A1Name, A1Default, A1Desc)               \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionXRegistrar<                                \
      Klass, decltype(static_cast<Ret (Klass::*)(A1)>(&Klass::Function)),      \
      &Klass::Function, Ret, A1>                                               \
      vrpc::detail::RegisterMemberFunctionX<                                   \
          Klass, decltype(static_cast<Ret (Klass::*)(A1)>(&Klass::Function)),  \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function,            \
                                                 FunctionDesc, {RetDesc},      \
                                                 {{A1Name, A1Default,          \
                                                   A1Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_9(Klass, Ret, RetDesc, Function,       \
                                        FunctionDesc, A1, A1Name, A1Default, \
                                        A1Desc)                              \
  template <>                                                                \
  const vrpc::detail::MemberFunctionXRegistrar<                              \
      Klass,                                                                 \
      decltype(static_cast<Ret (Klass::*)(A1) const>(&Klass::Function)),     \
      &Klass::Function, Ret, A1>                                             \
      vrpc::detail::RegisterMemberFunctionX<                                 \
          Klass,                                                             \
          decltype(static_cast<Ret (Klass::*)(A1) const>(&Klass::Function)), \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function,          \
                                                 FunctionDesc, {RetDesc},    \
                                                 {{A1Name, A1Default,        \
                                                   A1Desc}});

#define _VRPC_STATIC_FUNCTION_X_9(Klass, Ret, RetDesc, Function, FunctionDesc, \
                                  A1, A1Name, A1Default, A1Desc)               \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionXRegistrar<                                \
      decltype(static_cast<Ret (*)(A1)>(Klass::Function)), &Klass::Function,   \
      Ret, A1>                                                                 \
      vrpc::detail::RegisterStaticFunctionX<                                   \
          decltype(static_cast<Ret (*)(A1)>(Klass::Function)),                 \
          &Klass::Function, Ret, A1>::registerAs(#Klass, #Function,            \
                                                 FunctionDesc, {RetDesc},      \
                                                 {{A1Name, A1Default,          \
                                                   A1Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_3(Klass, Function, A1) \
  _VRPC_CONST_MEMBER_FUNCTION_4(Klass, void, Function, A1)

#define _VRPC_VOID_MEMBER_FUNCTION_3(Klass, Function, A1) \
  _VRPC_MEMBER_FUNCTION_4(Klass, void, Function, A1)

#define _VRPC_VOID_STATIC_FUNCTION_3(Klass, Function, A1) \
  _VRPC_STATIC_FUNCTION_4(Klass, void, Function, A1)

// ####################### Two arguments #######################

#define _VRPC_MEMBER_FUNCTION_5(Klass, Ret, Function, A1, A2)                 \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionRegistrar<                                \
      Klass, decltype(static_cast<Ret (Klass::*)(A1, A2)>(&Klass::Function)), \
      &Klass::Function, Ret, A1, A2>                                          \
      vrpc::detail::RegisterMemberFunction<                                   \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2)>(&Klass::Function)),    \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_5(Klass, Ret, Function, A1, A2)          \
  template <>                                                                \
  const vrpc::detail::MemberFunctionRegistrar<                               \
      Klass,                                                                 \
      decltype(static_cast<Ret (Klass::*)(A1, A2) const>(&Klass::Function)), \
      &Klass::Function, Ret, A1, A2>                                         \
      vrpc::detail::RegisterMemberFunction<                                  \
          Klass,                                                             \
          decltype(static_cast<Ret (Klass::*)(A1, A2) const>(                \
              &Klass::Function)),                                            \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function);

#define _VRPC_STATIC_FUNCTION_5(Klass, Ret, Function, A1, A2)      \
  template <>                                                      \
  const vrpc::detail::StaticFunctionRegistrar<                     \
      decltype(static_cast<Ret (*)(A1, A2)>(Klass::Function)),     \
      &Klass::Function, Ret, A1, A2>                               \
      vrpc::detail::RegisterStaticFunction<                        \
          decltype(static_cast<Ret (*)(A1, A2)>(Klass::Function)), \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function);

#define _VRPC_MEMBER_FUNCTION_X_13(Klass, Ret, RetDesc, Function,             \
                                   FunctionDesc, A1, A1Name, A1Default,       \
                                   A1Desc, A2, A2Name, A2Default, A2Desc)     \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass, decltype(static_cast<Ret (Klass::*)(A1, A2)>(&Klass::Function)), \
      &Klass::Function, Ret, A1, A2>                                          \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2)>(&Klass::Function)),    \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function,       \
                                                     FunctionDesc, {RetDesc}, \
                                                     {{A1Name, A1Default,     \
                                                       A1Desc},               \
                                                      {A2Name, A2Default,     \
                                                       A2Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_13(                                     \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc)                                    \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2) const>(&Klass::Function)),  \
      &Klass::Function, Ret, A1, A2>                                          \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2) const>(                 \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function,       \
                                                     FunctionDesc, {RetDesc}, \
                                                     {{A1Name, A1Default,     \
                                                       A1Desc},               \
                                                      {A2Name, A2Default,     \
                                                       A2Desc}});

#define _VRPC_STATIC_FUNCTION_X_13(Klass, Ret, RetDesc, Function,             \
                                   FunctionDesc, A1, A1Name, A1Default,       \
                                   A1Desc, A2, A2Name, A2Default, A2Desc)     \
  template <>                                                                 \
  const vrpc::detail::StaticFunctionXRegistrar<                               \
      decltype(static_cast<Ret (*)(A1, A2)>(Klass::Function)),                \
      &Klass::Function, Ret, A1, A2>                                          \
      vrpc::detail::RegisterStaticFunctionX<                                  \
          decltype(static_cast<Ret (*)(A1, A2)>(Klass::Function)),            \
          &Klass::Function, Ret, A1, A2>::registerAs(#Klass, #Function,       \
                                                     FunctionDesc, {RetDesc}, \
                                                     {{A1Name, A1Default,     \
                                                       A1Desc},               \
                                                      {A2Name, A2Default,     \
                                                       A2Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_4(Klass, Function, A1, A2) \
  _VRPC_CONST_MEMBER_FUNCTION_5(Klass, void, Function, A1, A2)

#define _VRPC_VOID_MEMBER_FUNCTION_4(Klass, Function, A1, A2) \
  _VRPC_MEMBER_FUNCTION_5(Klass, void, Function, A1, A2)

#define _VRPC_VOID_STATIC_FUNCTION_4(Klass, Function, A1, A2) \
  _VRPC_STATIC_FUNCTION_5(Klass, void, Function, A1, A2)

// ####################### Three arguments #######################

#define _VRPC_MEMBER_FUNCTION_6(Klass, Ret, Function, A1, A2, A3)              \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionRegistrar<                                 \
      Klass,                                                                   \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3)>(&Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3>                                       \
      vrpc::detail::RegisterMemberFunction<                                    \
          Klass,                                                               \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3)>(&Klass::Function)), \
          &Klass::Function, Ret, A1, A2, A3>::registerAs(#Klass, #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_6(Klass, Ret, Function, A1, A2, A3) \
  template <>                                                           \
  const vrpc::detail::MemberFunctionRegistrar<                          \
      Klass,                                                            \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3) const>(           \
          &Klass::Function)),                                           \
      &Klass::Function, Ret, A1, A2, A3>                                \
      vrpc::detail::RegisterMemberFunction<                             \
          Klass,                                                        \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3) const>(       \
              &Klass::Function)),                                       \
          &Klass::Function, Ret, A1, A2, A3>::registerAs(#Klass, #Function);

#define _VRPC_STATIC_FUNCTION_6(Klass, Ret, Function, A1, A2, A3)      \
  template <>                                                          \
  const vrpc::detail::StaticFunctionRegistrar<                         \
      decltype(static_cast<Ret (*)(A1, A2, A3)>(Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3>                               \
      vrpc::detail::RegisterStaticFunction<                            \
          decltype(static_cast<Ret (*)(A1, A2, A3)>(Klass::Function)), \
          &Klass::Function, Ret, A1, A2, A3>::registerAs(#Klass, #Function);

#define _VRPC_MEMBER_FUNCTION_X_17(                                            \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,        \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc)      \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionXRegistrar<                                \
      Klass,                                                                   \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3)>(&Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3>                                       \
      vrpc::detail::RegisterMemberFunctionX<                                   \
          Klass,                                                               \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3)>(&Klass::Function)), \
          &Klass::Function, Ret, A1, A2,                                       \
          A3>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},          \
                          {{A1Name, A1Default, A1Desc},                        \
                           {A2Name, A2Default, A2Desc},                        \
                           {A3Name, A3Default, A3Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_17(                                 \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,   \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc) \
  template <>                                                             \
  const vrpc::detail::MemberFunctionXRegistrar<                           \
      Klass,                                                              \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3) const>(             \
          &Klass::Function)),                                             \
      &Klass::Function, Ret, A1, A2, A3>                                  \
      vrpc::detail::RegisterMemberFunctionX<                              \
          Klass,                                                          \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3) const>(         \
              &Klass::Function)),                                         \
          &Klass::Function, Ret, A1, A2,                                  \
          A3>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},     \
                          {{A1Name, A1Default, A1Desc},                   \
                           {A2Name, A2Default, A2Desc},                   \
                           {A3Name, A3Default, A3Desc}});

#define _VRPC_STATIC_FUNCTION_X_17(                                       \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,   \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc) \
  template <>                                                             \
  const vrpc::detail::StaticFunctionXRegistrar<                           \
      decltype(static_cast<Ret (*)(A1, A2, A3)>(Klass::Function)),        \
      &Klass::Function, Ret, A1, A2, A3>                                  \
      vrpc::detail::RegisterStaticFunctionX<                              \
          decltype(static_cast<Ret (*)(A1, A2, A3)>(Klass::Function)),    \
          &Klass::Function, Ret, A1, A2,                                  \
          A3>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},     \
                          {{A1Name, A1Default, A1Desc},                   \
                           {A2Name, A2Default, A2Desc},                   \
                           {A3Name, A3Default, A3Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_5(Klass, Function, A1, A2, A3) \
  _VRPC_CONST_MEMBER_FUNCTION_6(Klass, void, Function, A1, A2, A3)

#define _VRPC_VOID_MEMBER_FUNCTION_5(Klass, Function, A1, A2, A3) \
  _VRPC_MEMBER_FUNCTION_6(Klass, void, Function, A1, A2, A3)

#define _VRPC_VOID_STATIC_FUNCTION_5(Klass, Function, A1, A2, A3) \
  _VRPC_STATIC_FUNCTION_6(Klass, void, Function, A1, A2, A3)

// ####################### Four arguments #######################

#define _VRPC_MEMBER_FUNCTION_7(Klass, Ret, Function, A1, A2, A3, A4)          \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionRegistrar<                                 \
      Klass,                                                                   \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4)>(&Klass::Function)), \
      &Klass::Function, Ret, A1, A2, A3, A4>                                   \
      vrpc::detail::RegisterMemberFunction<                                    \
          Klass,                                                               \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4)>(                \
              &Klass::Function)),                                              \
          &Klass::Function, Ret, A1, A2, A3, A4>::registerAs(#Klass,           \
                                                             #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_7(Klass, Ret, Function, A1, A2, A3, A4) \
  template <>                                                               \
  const vrpc::detail::MemberFunctionRegistrar<                              \
      Klass,                                                                \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4) const>(           \
          &Klass::Function)),                                               \
      &Klass::Function, Ret, A1, A2, A3, A4>                                \
      vrpc::detail::RegisterMemberFunction<                                 \
          Klass,                                                            \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4) const>(       \
              &Klass::Function)),                                           \
          &Klass::Function, Ret, A1, A2, A3, A4>::registerAs(#Klass,        \
                                                             #Function);

#define _VRPC_STATIC_FUNCTION_7(Klass, Ret, Function, A1, A2, A3, A4)      \
  template <>                                                              \
  const vrpc::detail::StaticFunctionRegistrar<                             \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3, A4>                               \
      vrpc::detail::RegisterStaticFunction<                                \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Klass::Function)), \
          &Klass::Function, Ret, A1, A2, A3, A4>::registerAs(#Klass,       \
                                                             #Function);

#define _VRPC_MEMBER_FUNCTION_X_21(                                            \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,        \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4,  \
    A4Name, A4Default, A4Desc)                                                 \
  template <>                                                                  \
  const vrpc::detail::MemberFunctionXRegistrar<                                \
      Klass,                                                                   \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4)>(&Klass::Function)), \
      &Klass::Function, Ret, A1, A2, A3, A4>                                   \
      vrpc::detail::RegisterMemberFunctionX<                                   \
          Klass,                                                               \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4)>(                \
              &Klass::Function)),                                              \
          &Klass::Function, Ret, A1, A2, A3,                                   \
          A4>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},          \
                          {{A1Name, A1Default, A1Desc},                        \
                           {A2Name, A2Default, A2Desc},                        \
                           {A3Name, A3Default, A3Desc},                        \
                           {A4NAme, A4Default, A4Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_21(                                     \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc)                                                \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4) const>(             \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4>                                  \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4) const>(         \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3,                                  \
          A4>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc}});

#define _VRPC_STATIC_FUNCTION_X_21(                                           \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc)                                                \
  template <>                                                                 \
  const vrpc::detail::StaticFunctionXRegistrar<                               \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Klass::Function)),        \
      &Klass::Function, Ret, A1, A2, A3, A4>                                  \
      vrpc::detail::RegisterStaticFunctionX<                                  \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4)>(Klass::Function)),    \
          &Klass::Function, Ret, A1, A2, A3,                                  \
          A4>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_6(Klass, Function, A1, A2, A3, A4) \
  _VRPC_CONST_MEMBER_FUNCTION_7(Klass, void, Function, A1, A2, A3, A4)

#define _VRPC_VOID_MEMBER_FUNCTION_6(Klass, Function, A1, A2, A3, A4) \
  _VRPC_MEMBER_FUNCTION_7(Klass, void, Function, A1, A2, A3, A4)

#define _VRPC_VOID_STATIC_FUNCTION_6(Klass, Function, A1, A2, A3, A4) \
  _VRPC_STATIC_FUNCTION_7(Klass, void, Function, A1, A2, A3, A4)

// ####################### Five arguments #######################

#define _VRPC_MEMBER_FUNCTION_8(Klass, Ret, Function, A1, A2, A3, A4, A5) \
  template <>                                                             \
  const vrpc::detail::MemberFunctionRegistrar<                            \
      Klass,                                                              \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5)>(           \
          &Klass::Function)),                                             \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                          \
      vrpc::detail::RegisterMemberFunction<                               \
          Klass,                                                          \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5)>(       \
              &Klass::Function)),                                         \
          &Klass::Function, Ret, A1, A2, A3, A4, A5>::registerAs(#Klass,  \
                                                                 #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_8(Klass, Ret, Function, A1, A2, A3, A4, \
                                      A5)                                   \
  template <>                                                               \
  const vrpc::detail::MemberFunctionRegistrar<                              \
      Klass,                                                                \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5) const>(       \
          &Klass::Function)),                                               \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                            \
      vrpc::detail::RegisterMemberFunction<                                 \
          Klass,                                                            \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5) const>(   \
              &Klass::Function)),                                           \
          &Klass::Function, Ret, A1, A2, A3, A4, A5>::registerAs(#Klass,    \
                                                                 #Function);

#define _VRPC_STATIC_FUNCTION_8(Klass, Ret, Function, A1, A2, A3, A4, A5)      \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionRegistrar<                                 \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                               \
      vrpc::detail::RegisterStaticFunction<                                    \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Klass::Function)), \
          &Klass::Function, Ret, A1, A2, A3, A4, A5>::registerAs(#Klass,       \
                                                                 #Function);

#define _VRPC_MEMBER_FUNCTION_X_25(                                           \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc)                 \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5)>(               \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                              \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5)>(           \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4,                              \
          A5>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc},                       \
                           {A5Name, A5Default, A5Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_25(                                     \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc)                 \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5) const>(         \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                              \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5) const>(     \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4,                              \
          A5>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc},                       \
                           {A5Name, A5Default, A5Desc}});

#define _VRPC_STATIC_FUNCTION_X_25(                                            \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,        \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4,  \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc)                  \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionXRegistrar<                                \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Klass::Function)),     \
      &Klass::Function, Ret, A1, A2, A3, A4, A5>                               \
      vrpc::detail::RegisterStaticFunctionX<                                   \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5)>(Klass::Function)), \
          &Klass::Function, Ret, A1, A2, A3, A4,                               \
          A5>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},          \
                          {{A1Name, A1Default, A1Desc},                        \
                           {A2Name, A2Default, A2Desc},                        \
                           {A3Name, A3Default, A3Desc},                        \
                           {A4NAme, A4Default, A4Desc},                        \
                           {A5Name, A5Default, A5Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_7(Klass, Function, A1, A2, A3, A4, \
                                           A5)                              \
  _VRPC_CONST_MEMBER_FUNCTION_8(Klass, void, Function, A1, A2, A3, A4, A5)

#define _VRPC_VOID_MEMBER_FUNCTION_7(Klass, Function, A1, A2, A3, A4, A5) \
  _VRPC_MEMBER_FUNCTION_8(Klass, void, Function, A1, A2, A3, A4, A5)

#define _VRPC_VOID_STATIC_FUNCTION_7(Klass, Function, A1, A2, A3, A4, A5) \
  _VRPC_STATIC_FUNCTION_8(Klass, void, Function, A1, A2, A3, A4, A5)

// ####################### Six arguments #######################

#define _VRPC_MEMBER_FUNCTION_9(Klass, Ret, Function, A1, A2, A3, A4, A5, A6) \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionRegistrar<                                \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6)>(           \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                          \
      vrpc::detail::RegisterMemberFunction<                                   \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6)>(       \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                          \
          A6>::registerAs(#Klass, #Function);

#define _VRPC_CONST_MEMBER_FUNCTION_9(Klass, Ret, Function, A1, A2, A3, A4,   \
                                      A5, A6)                                 \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionRegistrar<                                \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6) const>(     \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                          \
      vrpc::detail::RegisterMemberFunction<                                   \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6) const>( \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                          \
          A6>::registerAs(#Klass, #Function);

#define _VRPC_STATIC_FUNCTION_9(Klass, Ret, Function, A1, A2, A3, A4, A5, A6)  \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionRegistrar<                                 \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Klass::Function)), \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                           \
      vrpc::detail::RegisterStaticFunction<                                    \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(               \
              Klass::Function)),                                               \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                           \
          A6>::registerAs(#Klass, #Function);

#define _VRPC_MEMBER_FUNCTION_X_29(                                           \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc, A6, A6Name,     \
    A6Default, A6Desc)                                                        \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6)>(           \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                          \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6)>(       \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                          \
          A6>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc},                       \
                           {A5Name, A5Default, A5Desc},                       \
                           {A6Name, A6Default, A6Desc}});

#define _VRPC_CONST_MEMBER_FUNCTION_X_29(                                     \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,       \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4, \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc, A6, A6Name,     \
    A6Default, A6Desc)                                                        \
  template <>                                                                 \
  const vrpc::detail::MemberFunctionXRegistrar<                               \
      Klass,                                                                  \
      decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6) const>(     \
          &Klass::Function)),                                                 \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                          \
      vrpc::detail::RegisterMemberFunctionX<                                  \
          Klass,                                                              \
          decltype(static_cast<Ret (Klass::*)(A1, A2, A3, A4, A5, A6) const>( \
              &Klass::Function)),                                             \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                          \
          A6>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},         \
                          {{A1Name, A1Default, A1Desc},                       \
                           {A2Name, A2Default, A2Desc},                       \
                           {A3Name, A3Default, A3Desc},                       \
                           {A4NAme, A4Default, A4Desc},                       \
                           {A5Name, A5Default, A5Desc},                       \
                           {A6Name, A6Default, A6Desc}});

#define _VRPC_STATIC_FUNCTION_X_29(                                            \
    Klass, Ret, RetDesc, Function, FunctionDesc, A1, A1Name, A1Default,        \
    A1Desc, A2, A2Name, A2Default, A2Desc, A3, A3Name, A3Default, A3Desc, A4,  \
    A4Name, A4Default, A4Desc, A5, A5Name, A5Default, A5Desc, A6, A6Name,      \
    A6Default, A6Desc)                                                         \
  template <>                                                                  \
  const vrpc::detail::StaticFunctionXRegistrar<                                \
      decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(Klass::Function)), \
      &Klass::Function, Ret, A1, A2, A3, A4, A5, A6>                           \
      vrpc::detail::RegisterStaticFunctionX<                                   \
          decltype(static_cast<Ret (*)(A1, A2, A3, A4, A5, A6)>(               \
              Klass::Function)),                                               \
          &Klass::Function, Ret, A1, A2, A3, A4, A5,                           \
          A6>::registerAs(#Klass, #Function, FunctionDesc, {RetDesc},          \
                          {{A1Name, A1Default, A1Desc},                        \
                           {A2Name, A2Default, A2Desc},                        \
                           {A3Name, A3Default, A3Desc},                        \
                           {A4NAme, A4Default, A4Desc},                        \
                           {A5Name, A5Default, A5Desc},                        \
                           {A6Name, A6Default, A6Desc}});

#define _VRPC_VOID_MEMBER_FUNCTION_CONST_8(Klass, Function, A1, A2, A3, A4, \
                                           A5, A6)                          \
  _VRPC_CONST_MEMBER_FUNCTION_9(Klass, void, Function, A1, A2, A3, A4, A5, A6)

#define _VRPC_VOID_MEMBER_FUNCTION_8(Klass, Function, A1, A2, A3, A4, A5, A6) \
  _VRPC_MEMBER_FUNCTION_9(Klass, void, Function, A1, A2, A3, A4, A5, A6)

#define _VRPC_VOID_STATIC_FUNCTION_8(Klass, Function, A1, A2, A3, A4, A5, A6) \
  _VRPC_STATIC_FUNCTION_9(Klass, void, Function, A1, A2, A3, A4, A5, A6)

}  // namespace vrpc

#endif
