#include <vrpc/adapter.hpp>
#include <vrpc/agent.hpp>
#include "Foo.hpp"

namespace vrpc {
  VRPC_CTOR(Foo)
  VRPC_CTOR(Foo, int)
  VRPC_STATIC_FUNCTION(Foo, int, staticIncrement, int)
  VRPC_STATIC_FUNCTION(Foo, void, staticCallback, VRPC_CALLBACK(int), int)
  VRPC_MEMBER_FUNCTION(Foo, void, onValue, VRPC_CALLBACK(int))
  VRPC_MEMBER_FUNCTION(Foo, int, increment)
  VRPC_MEMBER_FUNCTION(Foo, void, reset)
  VRPC_MEMBER_FUNCTION(Foo, void, callback, VRPC_CALLBACK(int), int)
}

int main(int argc, char** argv) {
  auto agent = vrpc::VrpcAgent::from_commandline(argc, argv);
  if (agent) agent->serve();
  return EXIT_SUCCESS;
}
