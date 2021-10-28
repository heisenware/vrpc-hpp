#include <vrpc/vrpc_adapter.hpp>
#include <vrpc/vrpc_agent.hpp>
#include "Bar.hpp"

namespace vrpc {

// Register custom type: Bottle
void to_json(json& j, const Bottle& b) {
  j = json{{"name", b.name}, {"category", b.category}, {"country", b.country}};
}
void from_json(const json& j, Bottle& b) {
  b.name = j.at("name").get<std::string>();
  b.category = j.at("category").get<std::string>();
  b.country = j.at("country").get<std::string>();
}

// Register static function
VRPC_STATIC_FUNCTION(Bar, std::string, philosophy)

// Register constructors
VRPC_CTOR(Bar)
VRPC_CTOR(Bar, const Bar::Selection&)

// Register member functions
VRPC_MEMBER_FUNCTION_X(Bar,
                     void, "",
                     addBottle, "Adds a bottle to the bar",
                     const std::string&, "name", required(), "name of the bottle",
                     const std::string&, "category", "n/a", "category of the drink",
                     const std::string&, "country", "n/a", "country of production")
VRPC_MEMBER_FUNCTION(Bar, Bottle, removeBottle, const std::string&)
VRPC_MEMBER_FUNCTION(Bar, void, onAdd, VRPC_CALLBACK(const Bottle&))
VRPC_MEMBER_FUNCTION(Bar, void, onRemove, VRPC_CALLBACK(const Bottle&))
VRPC_CONST_MEMBER_FUNCTION(Bar, std::string, prepareDrink, VRPC_CALLBACK(const std::string&))
VRPC_CONST_MEMBER_FUNCTION(Bar, Bar::Selection, getSelection)

}  // namespace vrpc

int main(int argc, char** argv) {
  auto agent = vrpc::VrpcAgent::from_commandline(argc, argv);
  if (agent)
    agent->serve();
  return EXIT_SUCCESS;
}
