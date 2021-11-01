# C++ Adapter

When adapting existing C++ code, you must tell VRPC:

1. all classes and functions you want to use
2. all custom C++ data-types you want to expose

It is recommended to add the adapter code directly to the
file that implements the `main` function.

**IMPORTANT**
Always define adapter macros and functions within the VRPC namespace, i.e.

```cpp
namespace vrpc {
  // Adapter code goes here
}
```

---

VRPC basically uses only five different macro types to adapt existing code:

## 1. Constructors

```cpp
VRPC_CTOR(<className>, <args>)
```

Use this macro to register constructors with arguments. Repeat
this macro for all overloads you need.

## 2. Member functions

```cpp
VRPC_MEMBER_FUNCTION(<className>, <returnValue>, <functionName>[, <args>])
```

Use this macro to register class member functions. Repeat this macro for
all overloads you need.

For **const**ant member functions use:

```cpp
VRPC_CONST_MEMBER_FUNCTION(<className>, <returnValue>, <functionName>[, <args>])
```

## 3. Static functions

```cpp
VRPC_STATIC_FUNCTION(<className>, <returnValue>, <functionName>[, <args>])
```

  Use this macro to register static functions. Repeat this macro for all
  overloads you need.

## 4. Callbacks

```cpp
VRPC_CALLBACK(<args>)
```

Use this macro if an argument of a function you bind reflects a callback.
The provided arguments must match the expected signature of the callback.

## 5. Custom Types

```cpp
VRPC_DEFINE_TYPE(<type>, <member1>, <member2>, ...)
```

Use this macro to expose custom data types, such as structs.

---
**IMPORTANT** Windows is different

On windows the syntax of the macros is slightly different
(as the MSVC++ compiler is broken for variadic macros).
All member function and static function macros need to start with an
underscore and must end with `_<N>` where `N` is the number of arguments
in the macro.

To e.g. bind this member function of class `Foo`:

```cpp
std::string bar(int, float);
```

use:

```cpp
_VRPC_MEMBER_FUNCTION_5(Foo, std::string, bar, int, float)
```

---

## Adding function and parameter documentation

You may want to add function and parameter documentation to the bound functions
in order to help other users to understand how your API works.

**IMPORTANT** If you want to support **default values** for function arguments,
adding such meta information is essential!

Use a `_X` suffix to the binding macro which then allows for the following input:

```cpp
VRPC_MEMBER_FUNCTION_X(
  <className>,
  <returnValue>, "<return value description>",
  <functionName>, "<function description>"[,
  <arg1>, "<arg name>", <defaultValue> | required(), "<arg description>"[,
  ...]]
)
```

To e.g. bind this member function of class `Foo`:

```cpp
bool hasUser(const std::string& username, bool onlyLocal = false);
```

use:

```cpp
VRPC_MEMBER_FUNCTION_X(
  Foo,
  bool, "returns true if user was found, false otherwise",
  hasUser, "checks whether a given user exists",
  const std::string&, "username", required(), "the username to be checked",
  bool, "onlyLocal", false, "set true to only check local users"
)
```

**NOTE** Whenever your are using the `_X` suffix you must fully document the
return value, the function and each argument. There is no possibility to skip
any documentation information.

## Adapting custom data types

Say your existing code had a `struct`:

```cpp
// a simple struct to model a person
struct Person {
  std::string name;
  std::string address;
  int age;
};
```

Then on the top of your adapter file (before all other macros) add:

```cpp
namespace vrpc {
  VRPC_DEFINE_TYPE(Person, name, address, age);
  // other macros follow here...
}
```

Once you exposed your custom data-types you are ready to use them as
arguments in the binding macros (see above). They automatically also work within
all STL containers and even as arguments of callback functions!

This feature is brought in by the library [JSON for Modern
C++](https://github.com/nlohmann/json) which is embedded in VRPC and
depending on your needs you may want instead to define the low-level `to_json`
and `from_json` functions. In that case just see there for more details.
