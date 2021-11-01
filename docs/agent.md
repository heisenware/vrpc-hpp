# C++ Agent

- Defined in header `<vrpc/agent.hpp>`
- Provides class: `vrpc::VrpcAgent`

The `vrpc::VrpcAgent` establishes a connection to an MQTT broker and makes
all classes and function that are bound through an `binding.cpp` file (see above)
remotely available.

* * *

## Options Struct


```cpp
struct Options {
  std::string domain = "public.vrpc";
  std::string agent;
  std::string token;
  std::string plugin;
  std::string username;
  std::string password;
  std::string persistence_directory;
  std::string trust_store;
  std::string key_store;
  std::string private_key;
  std::string private_key_password;
  std::string enabled_cipher_suites;
  std::string broker = "ssl://vrpc.io:8883";
  bool enable_server_cert_auth = false;
};
```

Simple struct holding configuration options needed for `VrpcAgent` construction.

## Static functions

```cpp
std::shared_ptr<vrpc::VrpcAgent> vrpc::VrpcAgent::from_commandline( int argc, char** argv );
```

Creates a VrpcAgent instance from commandline parameters. The corresponding
arguments of the `main` function can simply be handed over.

**Parameters**

`argc` - Argument count

`argv` - Array of argument values

**Return Value**

Shared pointer to an VrpcAgent instance. Will be a null pointer in case
of wrong commandline arguments or if the help function was triggered.

* * *

```cpp
std::shared_ptr<vrpc::VrpcAgent> vrpc::VrpcAgent::create( const Options& options );
```

Creates a VrpcAgent instance from provided options.

**Parameters**

`options` - Option object providing configuration information

**Return Value**

Shared pointer to an VrpcAgent instance.

## Member functions

```cpp
void serve();
```

Tries to establish a connection to the configured MQTT broker (bound to vrpc.io
in the community edition) and if successful starts an underlying event-loop.

If not successful, `serve` tries re-connecting to the broker.

**NOTE**: This function is blocking, but can be continued by the signals`SIGINT`,
`SIGTERM` or `SIGSEV`, which stops the event-loop.

**Example**

```cpp
#include <vrpc/agent.hpp>

int main(int argc, char** argv) {
  auto agent = VrpcAgent::from_commandline(argc, argv);
  if (agent) agent->serve();
  return EXIT_SUCCESS;
}
```
