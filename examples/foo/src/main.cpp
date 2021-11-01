#include <vrpc/adapter.hpp>
#include <vrpc/agent.hpp>

#include "Foo.hpp"

// always define the adapter macros in the vrpc namespace
namespace vrpc {

// Adapt constructor
// Needs: class, [arg1-type, [arg2-type...]]
VRPC_CTOR(Foo);

// Adapt const member function
// Needs: class, return-type, function, [arg1-type, [arg2-type...]]
VRPC_CONST_MEMBER_FUNCTION(Foo, int, getValue)

// Adapt non-const member function
// Needs: class, return-type, function, [arg1-type, [arg2-type...]]
VRPC_MEMBER_FUNCTION(Foo, void, setValue, int)
}  // namespace vrpc

int main(int argc, char** argv) {
  auto agent = vrpc::VrpcAgent::from_commandline(argc, argv);
  if (agent) agent->serve();
  return EXIT_SUCCESS;
}
