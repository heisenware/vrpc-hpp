# VRPC C++ Agent

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++14](https://img.shields.io/badge/C++-14-blue.svg)](https://isocpp.org/)

**Stop writing API boilerplate.** VRPC (Virtual Remote Procedure Call) allows you to call C++, Node.js, Python, and Arduino classes across any network as if they were local objects. It is perfect for microservices, IoT edge devices, and directly driving React frontends—without the need for REST, GraphQL, or WebSocket boilerplate.

This repository provides the **C++ Agent**, allowing you to non-intrusively bind your existing C++ code and make it remotely controllable in minutes.

---

## Why VRPC for C++?

* **Zero Boilerplate:** No IDL files (like Protobuf or gRPC), no route definitions, and no payload parsing. Just use simple macros to register your class, and VRPC instantly makes its public methods remotely callable.
* **Native Event Proxies:** Don't just fetch data—stream it. VRPC transparently proxies C++ callbacks across the network. When your C++ edge device emits a hardware event, your React UI or Node.js backend updates instantly.
* **MQTT-Powered NAT Traversal:** Built on top of robust MQTT, VRPC agents make outbound connections to your broker. No open firewall ports, no complex reverse proxies, and perfect resilience on unstable edge networks.
* **Header-Only Core:** The binding adapter is lightweight and highly template-driven, seamlessly handling variadic arguments and complex data types via JSON serialization.

## Quick Start

With VRPC, making a C++ class remotely accessible requires almost zero changes to your actual business logic. As a modern header-only library, integrating it into your project is trivial.

### 1. Integrate via CMake
Add `vrpc-hpp` to your `CMakeLists.txt` using `FetchContent`. We safely bundle the necessary Boost dependencies by default, so it just works out of the box!

```cmake
cmake_minimum_required(VERSION 3.14)
project(vrpc_example LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  vrpc
  GIT_REPOSITORY [https://github.com/heisenware/vrpc-hpp.git](https://github.com/heisenware/vrpc-hpp.git)
  GIT_TAG        v3.1.0 # Or use 'main' for the latest
)
FetchContent_MakeAvailable(vrpc)

add_executable(my_vrpc_agent main.cpp adapter.cpp)

# Link your target against vrpc::vrpc (handles threads, includes, and dependencies automatically)
target_link_libraries(my_vrpc_agent PRIVATE vrpc::vrpc)
```

*(Note: If you have your own system Boost installed, VRPC will automatically detect and use yours instead to prevent conflicts!)*

### 2. Write your C++ Class
```cpp
// Foo.hpp
#include <iostream>
#include <functional>

class Foo {
  int _value;

public:
  Foo(int initial_value) : _value(initial_value) {}

  int increment() {
    return ++_value;
  }

  // VRPC supports asynchronous callbacks out of the box!
  void onValue(const std::function<void(int)>& callback) {
     callback(_value);
  }
};
```

### 3. Bind it using VRPC
```cpp
// adapter.cpp
#include <vrpc/adapter.hpp>
#include "Foo.hpp"

// Bind the constructor
VRPC_CTOR(Foo, int)

// Bind the member functions
VRPC_MEMBER_FUNCTION(Foo, int, increment)
VRPC_MEMBER_FUNCTION(Foo, void, onValue, VRPC_CALLBACK(int))
```

### 4. Start the Agent
```cpp
// main.cpp
#include <vrpc/agent.hpp>

int main(int argc, char** argv) {
  auto agent = vrpc::VrpcAgent::from_commandline(argc, argv);
  if (agent) {
    agent->serve();
  }
  return 0;
}
```

Build your project and start your agent from the command line:
```bash
mkdir build && cd build
cmake ..
make

./my_vrpc_agent -d my_domain -a cpp_edge_device -b mqtts://broker.hivemq.com:8883
```

### 5. Call it from Anywhere (e.g., Node.js / React)
Once your C++ agent is running, you can interact with it transparently from any VRPC client:

```javascript
import { VrpcClient } from 'vrpc';

const client = new VrpcClient({ domain: 'my_domain' });

await client.connect();

// Create a remote instance of your C++ class
const foo = await client.create({
  agent: 'cpp_edge_device',
  className: 'Foo',
  args: [41]
});

// Call functions natively
const result = await foo.increment();
console.log(result); // 42

// Listen to C++ callbacks across the network!
await foo.onValue((val) => {
  console.log(`C++ emitted: ${val}`);
});
```

## The VRPC Ecosystem

Write your performance-critical code in **C++**, your data-science scripts in **Python**, your business logic in **Node.js**, and your IoT firmware on **Arduino**. Call them all identically.

* [VRPC for Node.js / Browser](https://github.com/heisenware/vrpc-js)
* [VRPC for Python](https://github.com/heisenware/vrpc-python)
* [VRPC for Arduino / ESP32](https://github.com/heisenware/vrpc-arduino)
* [VRPC for React](https://github.com/heisenware/vrpc-react)

## Documentation

For detailed installation instructions, CMake integration, advanced macro usage, and architecture overviews, please visit our official documentation at **[vrpc.io/docs](https://vrpc.io/docs)**.

## Contributing

Contributions are welcome! Whether it's reporting a bug, proposing a new feature, or submitting a pull request, we'd love your help to make VRPC even better. Please read our [Contributing Guidelines](CONTRIBUTING.md) to get started.

## License

VRPC is released under the [MIT License](LICENSE).
