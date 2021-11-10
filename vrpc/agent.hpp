#include <bitset>
#include <iomanip>
#include <iostream>
#include <map>

#include <vrpc/adapter.hpp>
#include <vrpc/json.hpp>
#include <vrpc/mqtt.hpp>

using namespace std::chrono_literals;

#define VRPC_PROTOCOL_VERSION 3

namespace vrpc {

class VrpcAgent {
  struct BrokerInfo {
    std::string host;
    std::string port;
    bool is_ssl;
  };

  std::string _domain;
  std::string _agent;
  std::string _username;
  std::string _password;
  std::string _token;
  std::string _version;
  std::string _url;
  std::string _plugin;
  BrokerInfo _broker;

  // Isolated instances
  typedef std::unordered_map<std::string,
                             std::set<std::pair<std::string, std::string>>>
      IsolatedInstances;
  IsolatedInstances _isolated_instances;

  // mqtt client
  typedef std::shared_ptr<mqtt::callable_overlay<mqtt::sync_client<
      mqtt::tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
      Client;

  Client _client;

  using packet_id_t =
      typename std::remove_reference_t<decltype(*_client)>::packet_id_t;

  // event loop
  boost::asio::io_context _ioc;

 public:
  // agent configuration
  struct Options {
    std::string agent;
    std::string username;
    std::string password;
    std::string token;
    std::string version;
    std::string plugin;
    std::string domain = "public.vrpc";
    std::string broker = "tcp://vrpc.io:1883";
  };

  static std::shared_ptr<VrpcAgent> from_commandline(int argc, char** argv) {
    Options opts;
    std::vector<std::string> args(argv + 1, argv + argc);
    if (VrpcAgent::parse_commandline(args, opts)) {
      if (opts.agent == "") {
        opts.agent = VrpcAgent::generate_agent_name();
      }
      return std::shared_ptr<VrpcAgent>(new VrpcAgent(opts));
    }
    return std::shared_ptr<VrpcAgent>();
  }

  static std::shared_ptr<VrpcAgent> create(const Options& options) {
    return std::shared_ptr<VrpcAgent>(new VrpcAgent(options));
  }

  void serve() {
    std::cout << "Domain : " << _domain << std::endl;
    std::cout << "Agent  : " << _agent << std::endl;
    std::cout << "Broker : " << _url << std::endl;

    // setup client
    if (!_username.empty()) {
      _client->set_user_name(_username);
    }
    if (!_password.empty()) {
      _client->set_password(_password);
    }
    if (!_token.empty()) {
      _client->set_user_name("__token__");
      _client->set_password(_token);
    }
    _client->set_client_id(generate_client_id());
    _client->set_clean_session(true);
    _client->set_will(mqtt::will(
        mqtt::allocate_buffer(_domain + "/" + _agent + "/__agentInfo__"),
        mqtt::allocate_buffer(
            json{{"status", "offline"}, {"hostname", VrpcAgent::get_hostname()}}
                .dump()),
        mqtt::qos::at_least_once | mqtt::retain::yes));

    // reconnect handler
    boost::asio::steady_timer timer(_ioc);
    std::function<void()> reconnect;
    reconnect = [&] {
      // wait 3 seconds and connect again
      timer.expires_after(std::chrono::seconds(3));
      timer.async_wait([&](boost::system::error_code const& ec) {
        if (!ec) {
          // timer fired
          std::cout << "try connect again" << std::endl;
          mqtt::error_code ec;
          _client->connect(ec);  // connect again
          if (ec) {
            std::cout << "error " << ec.message() << std::endl;
            reconnect();
          }
        }
      });
    };

    // on connection established
    _client->set_connack_handler(
        [&](bool sp, mqtt::connect_return_code connack_return_code) {
          if (connack_return_code == mqtt::connect_return_code::accepted) {
            std::cout << "[OK]" << std::endl;
            publish_agent_info();
            auto topics = generate_topics();
            for (const auto& topic : topics) {
              _client->subscribe(topic, mqtt::qos::at_least_once);
            }
            // Publish class information
            const auto& classes = LocalFactory::get_classes();
            for (const auto& klass : classes) {
              publish_class_info(klass);
            }
          }
          return true;
        });

    // on message arrived
    _client->set_publish_handler([&](mqtt::optional<packet_id_t> packet_id,
                                     mqtt::publish_options pubopts,
                                     mqtt::buffer topic_b,
                                     mqtt::buffer contents) {
      _VRPC_DEBUG << "message received." << std::endl;
      _VRPC_DEBUG << "topic: " << topic_b << std::endl;
      _VRPC_DEBUG << "contents: " << contents << std::endl;

      const std::string topic(topic_b);
      const auto tokens = VrpcAgent::tokenize(topic, "/");

      // Special case: clientInfo message
      if (tokens.size() == 4 && tokens[3] == "__clientInfo__") {
        auto j = json::parse(std::string(contents));
        if (j["status"].get<std::string>() == "offline") {
          const std::string client = topic.substr(0, topic.length() - 9);
          const auto it = _isolated_instances.find(client);
          if (it != _isolated_instances.end()) {
            for (const auto& p : it->second) {
              json j{{"c", p.second},
                     {"f", "__delete__"},
                     {"a", json::array({p.first})}};
              LocalFactory::call(j);
            }
            _client->unsubscribe(client + "/__clientInfo__");
          }
        }
        return true;
      }

      if (tokens.size() != 5) {
        std::cerr << "Received message with invalid topic URI" << std::endl;
        return true;
      }

      // call execution will be handled by the event-loop
      boost::asio::post(_ioc, [&, contents, tokens]() {
        // TODO try witout string
        auto j = json::parse(std::string(contents));
        const std::string klass = tokens[2];
        const std::string instance = tokens[3];
        const std::string function = tokens[4];
        const std::string sender = j.at("s");
        // set context for correct call
        j["c"] = instance == "__static__" ? klass : instance;
        // set function
        j["f"] = function;

        // The actual call to the existing code
        // NOTE: the json object is mutated during the call
        LocalFactory::call(j);

        // Lifetime handling
        if (function == "__createIsolated__") {
          // Return value = instance
          const std::string instance(j["r"].get<std::string>());
          subscribe_to_instance(klass, instance);
          register_isolated_instance(instance, klass, sender);
        } else if (function == "__createShared__") {
          const std::string instance(j["r"].get<std::string>());
          subscribe_to_instance(klass, instance);
          // Publish classInfo message (as number of instances changed)
          publish_class_info(klass);
        } else if (function == "__delete__") {
          unsubscribe_from_instance(klass, instance);
          publish_class_info(klass);
          unregister_isolated_instance(instance, klass, sender);
        }
        // RPC answer goes here
        _client->publish(sender, j.dump(), mqtt::qos::at_least_once);
      });
      return true;
    });

    // on connection closed
    _client->set_close_handler([&] {
      std::cout << "connection closed" << std::endl;
      reconnect();
    });

    // on error
    _client->set_error_handler([&](boost::system::error_code const& ec) {
      std::cout << "connection error " << ec.message() << std::endl;
      reconnect();
    });

    // Connect
    mqtt::error_code ec;
    _client->connect(ec);
    if (ec) {
      std::cout << "error " << ec.message() << std::endl;
      reconnect();
    }
    _ioc.run();
  }

  void end() {
    _client->publish(_domain + "/" + _agent + "/__agentInfo__",
                     json{{"status", "offline"},
                          {"hostname", VrpcAgent::get_hostname()},
                          {"v", VRPC_PROTOCOL_VERSION}}
                         .dump(),
                     mqtt::qos::at_least_once | mqtt::retain::yes);
    _client->disconnect(3s);
    _ioc.stop();
  }

 private:
  VrpcAgent(const Options& options)
      : _domain(options.domain),
        _agent(options.agent),
        _username(options.username),
        _password(options.password),
        _token(options.token),
        _version(options.version),
        _url(options.broker),
        _broker(VrpcAgent::extract_broker_info(options.broker)) {
    // TODO validate domain and agent
    if (_agent.empty()) {
      _agent = VrpcAgent::generate_agent_name();
    }
    // instantiate mqtt client
    _client = mqtt::make_sync_client(_ioc, _broker.host, _broker.port);
    // register handler for vrpc callbacks
    vrpc::Callback::register_callback_handler([&](const json& j) {
      const std::string sender = j["s"].get<std::string>();
      _client->publish(sender, j.dump(), mqtt::qos::at_least_once);
    });
  }

  static bool parse_commandline(const std::vector<std::string>& args,
                                Options& opts) {
    const size_t size = args.size();
    for (size_t i = 0; i < size; ++i) {
      const std::string arg(args[i]);
      if (arg == "--help") {
        std::cout << "usage: " << args[0]
                  << " -d <domain> -a <agent> -t <token> -u <user> -p "
                     "<password> -b <broker> -l <load-plugin>"
                  << std::endl;
        return false;
      }
      if (arg == "--version") {
        std::cout << "3.0.0" << std::endl;
        return false;
      }
      if (arg == "-d" && ++i < size) {
        opts.domain = args[i];
      } else if (arg == "-a" && ++i < size) {
        opts.agent = args[i];
      } else if (arg == "-t" && ++i < size) {
        opts.token = args[i];
      } else if (arg == "-p" && ++i < size) {
        opts.plugin = args[i];
      } else if (arg == "-v" && ++i < size) {
        opts.version = args[i];
      } else if (arg == "-b" && ++i < size) {
        opts.broker = args[i];
      } else if (arg == "-u" && ++i < size) {
        opts.username = args[i];
      } else if (arg == "-p" && ++i < size) {
        opts.password = args[i];
      }
    }
    return true;
  }

  static BrokerInfo extract_broker_info(const std::string& broker) {
    const size_t pos1 = broker.find_first_of("://");
    if (pos1 == std::string::npos) {
      throw std::runtime_error(
          "Missing scheme in broker url (use e.g. mqtts://<hostname>)");
    }
    BrokerInfo info;
    info.is_ssl =
        broker.substr(0, pos1) == "ssl" || broker.substr(0, pos1) == "mqtts";
    info.host = broker.substr(pos1 + 3);
    const size_t pos2 = info.host.find_first_of(":");
    if (pos2 == std::string::npos) {
      info.port = info.is_ssl ? "8883" : "1883";
    } else {
      info.port = info.host.substr(pos2 + 1);
      info.host = info.host.substr(0, pos2);
    }
    return info;
  }

  static std::string generate_agent_name() {
    const std::string username(get_username());
    // TODO once c++17 is standard we will use filesystem::current_path() here
    const std::string pathId(get_current_path_id());
    const std::string hostname(get_hostname());
    const std::string platform(get_platform());
    return username + "-" + pathId + "@" + hostname + "-" + platform + "-cpp";
  }

  static std::string get_username() {
#ifdef __linux__
    char username[LOGIN_NAME_MAX];
    getlogin_r(username, LOGIN_NAME_MAX);
#else
    std::string username('unkown')
#endif
    return std::string(username);
  }

  static std::string get_current_path_id() {
    const std::string path(get_current_path());
    return std::to_string(std::hash<std::string>{}(path)).substr(0, 4);
  }

  static std::string get_current_path() {
#ifdef __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#else
    return detail::get_hostname()
#endif
  }

  static std::string get_hostname() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return std::string(hostname);
  }

  static std::string get_platform() {
#ifdef _WIN32
    return "win32";
#elif _WIN64
    return "win64";
#elif __APPLE__ || __MACH__
    return "darwin";
#elif __linux__
    return "linux";
#elif __FreeBSD__
    return "freebsd";
#elif __unix || __unix__
    return "unix";
#else
    return "other";
#endif
  }

  std::string generate_client_id() {
    const std::string h = _domain + _agent;
    std::to_string(std::hash<std::string>{}(h)).substr(0, 18);
    return "vrpca" + std::to_string(std::hash<std::string>{}(h)).substr(0, 18);
  }

  void publish_agent_info() {
    json j;
    j["status"] = "online";
    j["hostname"] = VrpcAgent::get_hostname();
    j["version"] = _version;
    j["v"] = VRPC_PROTOCOL_VERSION;
    const std::string t(_domain + "/" + _agent + "/__agentInfo__");
    _client->publish(t, j.dump(), mqtt::qos::at_least_once | mqtt::retain::yes);
  }

  std::vector<std::string> generate_topics() const {
    std::vector<std::string> topics;
    // Register all static functions
    const auto& classes = LocalFactory::get_classes();
    for (const auto& klass : classes) {
      const auto& functions = LocalFactory::get_static_functions(klass);
      std::set<std::string> no_dups;
      for (const auto& func : functions) {
        const std::string func_base = VrpcAgent::remove_signature(func);
        if (no_dups.find(func_base) != no_dups.end()) {
          continue;
        }
        no_dups.insert(func_base);
        const std::string topic(_domain + "/" + _agent + "/" + klass +
                                "/__static__/" + func_base);
        topics.push_back(topic);
        _VRPC_DEBUG << "Preparing to have topic: " << topic << std::endl;
      }
    }
    return topics;
  }

  static std::string remove_signature(const std::string& function) {
    const size_t pos = function.find_first_of("-");
    return pos == std::string::npos ? function : function.substr(0, pos);
  }

  void publish_class_info(const std::string& klass) {
    json j;
    j["className"] = klass;
    j["instances"] = LocalFactory::get_instances(klass);
    j["memberFunctions"] = LocalFactory::get_member_functions(klass);
    j["staticFunctions"] = LocalFactory::get_static_functions(klass);
    j["meta"] = LocalFactory::get_meta_data(klass);
    j["v"] = VRPC_PROTOCOL_VERSION;
    std::string t(_domain + "/" + _agent + "/" + klass + "/__classInfo__");
    _client->publish(t, j.dump(), mqtt::qos::at_least_once | mqtt::retain::yes);
  }

  static std::vector<std::string> tokenize(const std::string& input,
                                           char const* delimiters) {
    std::vector<std::string> output;
    std::bitset<255> delims;
    while (*delimiters) {
      unsigned char code = *delimiters++;
      delims[code] = true;
    }
    std::string::const_iterator beg;
    bool in_token = false;
    for (std::string::const_iterator it = input.begin(), end = input.end();
         it != end; ++it) {
      if (delims[*it & 0xff]) {
        if (in_token) {
          output.push_back(std::string(beg, it));
          in_token = false;
        }
      } else if (!in_token) {
        beg = it;
        in_token = true;
      }
    }
    if (in_token) {
      output.push_back(std::string(beg, input.end()));
    }
    return output;
  }

  void subscribe_to_instance(const std::string& klass,
                             const std::string& instance) {
    for (auto function : LocalFactory::get_member_functions(klass)) {
      function = VrpcAgent::remove_signature(function);
      const std::string topic(_domain + "/" + _agent + "/" + klass + "/" +
                              instance + "/" + function);
      _client->subscribe(topic, mqtt::qos::at_least_once);
      _VRPC_DEBUG << "Subscribed to new topic: " << topic << std::endl;
    }
  }

  void unsubscribe_from_instance(const std::string& klass,
                                 const std::string& instance) {
    for (auto function : LocalFactory::get_member_functions(klass)) {
      function = VrpcAgent::remove_signature(function);
      const std::string topic(_domain + "/" + _agent + "/" + klass + "/" +
                              instance + "/" + function);
      _client->unsubscribe(topic);
      _VRPC_DEBUG << "Unsubscribed from topic after deletion: " << topic
                  << std::endl;
    }
  }

  void register_isolated_instance(const std::string& instance,
                                  const std::string& klass,
                                  const std::string& client_id) {
    const auto it = _isolated_instances.find(client_id);
    if (it != _isolated_instances.end()) {  // already seen
      it->second.insert({instance, klass});
    } else {  // new client
      _isolated_instances[client_id].insert({instance, klass});
      _client->subscribe(client_id + "/__clientInfo__",
                         mqtt::qos::at_least_once);
    }
    _VRPC_DEBUG << "Tracking lifetime of client: " << client_id << std::endl;
  }

  void unregister_isolated_instance(const std::string& instance,
                                    const std::string& klass,
                                    const std::string& client_id) {
    const auto it = _isolated_instances.find(client_id);
    if (it == _isolated_instances.end()) {
      _VRPC_DEBUG << "Failed un-registerting not registered instance: "
                  << instance << " on client: " << client_id << std::endl;
      return;
    }
    it->second.erase({instance, klass});
    if (it->second.size() == 0) {
      _client->unsubscribe(client_id + "/__clientInfo__");
      _VRPC_DEBUG << "Stopped tracking lifetime of client: " << client_id
                  << std::endl;
    }
  }
};
}  // namespace vrpc
