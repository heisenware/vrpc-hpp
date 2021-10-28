// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_ASYNC_CLIENT_HPP)
#define MQTT_ASYNC_CLIENT_HPP

#include <vrpc/mqtt/namespace.hpp>
#include <vrpc/mqtt/client.hpp>
#include <vrpc/mqtt/move.hpp>
#include <vrpc/mqtt/callable_overlay.hpp>

namespace MQTT_NS {

template <typename Socket, std::size_t PacketIdBytes = 2>
class async_client : public client<Socket, PacketIdBytes> {
    using this_type = async_client<Socket, PacketIdBytes>;
    using base = client<Socket, PacketIdBytes>;
    using constructor_access = typename base::constructor_access;
public:

    /**
     * Constructor used by factory functions at the end of this file.
     */
    template<typename ... Args>
    async_client(constructor_access, Args && ... args)
     : async_client(std::forward<Args>(args)...)
    { }

    /**
     * @brief Create no tls async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
    make_async_client(as::io_context& ioc, std::string host, std::string port, protocol_version version);

    /**
     * @brief Create no tls async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>>>>
    make_async_client_no_strand(as::io_context& ioc, std::string host, std::string port, protocol_version version);

#if defined(MQTT_USE_WS)
    /**
     * @brief Create no tls websocket async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object.
     *  strand is controlled by ws_endpoint, not endpoint, so async_client has null_strand template argument.
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
    make_async_client_ws(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);

    /**
     * @brief Create no tls websocket async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>>>>
    make_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);
#endif // defined(MQTT_USE_WS)

#if defined(MQTT_USE_TLS)
    /**
     * @brief Create tls async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
    make_tls_async_client(as::io_context& ioc, std::string host, std::string port, protocol_version version);

    /**
     * @brief Create tls async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
    make_tls_async_client_no_strand(as::io_context& ioc, std::string host, std::string port, protocol_version version);

#if defined(MQTT_USE_WS)
    /**
     * @brief Create no tls websocket async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object.
     *  strand is controlled by ws_endpoint, not endpoint, so async_client has null_strand template argument.
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
    make_tls_async_client_ws(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);

    /**
     * @brief Create no tls websocket async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
    make_tls_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);
#endif // defined(MQTT_USE_WS)
#endif // defined(MQTT_USE_TLS)

    /**
     * @brief Create no tls async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
    make_async_client_32(as::io_context& ioc, std::string host, std::string port, protocol_version version);

    /**
     * @brief Create no tls async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
    make_async_client_no_strand_32(as::io_context& ioc, std::string host, std::string port, protocol_version version);

#if defined(MQTT_USE_WS)
    /**
     * @brief Create no tls websocket async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object.
     *  strand is controlled by ws_endpoint, not endpoint, so async_client has null_strand template argument.
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
    make_async_client_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);

    /**
     * @brief Create no tls websocket async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
    make_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);
#endif // defined(MQTT_USE_WS)

#if defined(MQTT_USE_TLS)
    /**
     * @brief Create tls async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
    make_tls_async_client_32(as::io_context& ioc, std::string host, std::string port, protocol_version version);

    /**
     * @brief Create tls async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
    make_tls_async_client_no_strand_32(as::io_context& ioc, std::string host, std::string port, protocol_version version);

#if defined(MQTT_USE_WS)
    /**
     * @brief Create no tls websocket async_client with strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object.
     *  strand is controlled by ws_endpoint, not endpoint, so async_client has null_strand template argument.
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
    make_tls_async_client_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);

    /**
     * @brief Create no tls websocket async_client without strand.
     * @param ioc io_context object.
     * @param host hostname
     * @param port port number
     * @param path path string
     * @return async_client object
     */
    friend std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
    make_tls_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path, protocol_version version);
#endif // defined(MQTT_USE_WS)
#endif // defined(MQTT_USE_TLS)


    /**
     * @brief Set auto publish response mode.
     * @param b set value
     * @param async auto publish response send asynchronous
     *
     * When set auto publish response mode to true, puback, pubrec, pubrel,and pub comp automatically send.<BR>
     */
    void set_auto_pub_response(bool b = true) {
        base::set_auto_pub_response(b, true);
    }

    void connect() = delete;
    void disconnect() = delete;

    void publish() = delete;
    void subscribe() = delete;
    void unsubscribe() = delete;
    void pingresp() = delete;
    void connack() = delete;
    void puback() = delete;
    void pubrec() = delete;
    void pubrel() = delete;
    void pubcomp() = delete;
    void suback() = delete;
    void unsuback() = delete;

protected:
    // Ensure that only code that knows the *exact* type of an object
    // inheriting from this abstract base class can destruct it.
    // This avoids issues of the destructor not triggering destruction
    // of derived classes, and any member variables contained in them.
    // Note: Not virtual to avoid need for a vtable when possible.
    ~async_client() = default;

    async_client(
        as::io_context& ioc,
        std::string host,
        std::string port,
#if defined(MQTT_USE_WS)
        std::string path = "/",
#endif // defined(MQTT_USE_WS)
        protocol_version version = protocol_version::v3_1_1
    ):base(ioc,
           force_move(host),
           force_move(port),
#if defined(MQTT_USE_WS)
           force_move(path),
#endif // defined(MQTT_USE_WS)
           version,
           true
    ) {
        set_auto_pub_response();
        base::set_async_pingreq(true);
    }
};

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
make_async_client(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
make_async_client(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_async_client(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>>>>
make_async_client_no_strand(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>>>>
make_async_client_no_strand(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_no_strand(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

#if defined(MQTT_USE_WS)

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
make_async_client_ws(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>
make_async_client_ws(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_ws(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>>>>
make_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<as::ip::tcp::socket, null_strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>>>>
make_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_no_strand_ws(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

#endif // defined(MQTT_USE_WS)

#if defined(MQTT_USE_TLS)

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
make_tls_async_client(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
make_tls_async_client(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
make_tls_async_client_no_strand(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
make_tls_async_client_no_strand(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_no_strand(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

#if defined(MQTT_USE_WS)

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
make_tls_async_client_ws(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>>>>
make_tls_async_client_ws(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_ws(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
make_tls_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>>>>
make_tls_async_client_no_strand_ws(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_no_strand_ws(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

#endif // defined(MQTT_USE_WS)

#endif // defined(MQTT_USE_TLS)


// 32bit Packet Id (experimental)

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
make_async_client_32(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
make_async_client_32(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_32(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
make_async_client_no_strand_32(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
make_async_client_no_strand_32(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_no_strand_32(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

#if defined(MQTT_USE_WS)

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
make_async_client_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, as::io_context::strand>, 4>>>
make_async_client_ws_32(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_ws_32(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
make_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<as::ip::tcp::socket, null_strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<as::ip::tcp::socket, null_strand>, 4>>>
make_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_async_client_no_strand_ws_32(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

#endif // defined(MQTT_USE_WS)

#if defined(MQTT_USE_TLS)

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
make_tls_async_client_32(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
make_tls_async_client_32(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_32(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
make_tls_async_client_no_strand_32(as::io_context& ioc, std::string host, std::string port, protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
#if defined(MQTT_USE_WS)
        "/",
#endif // defined(MQTT_USE_WS)
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<tcp_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
make_tls_async_client_no_strand_32(as::io_context& ioc, std::string host, std::uint16_t port, protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_no_strand_32(
        ioc,
        force_move(host),
        std::to_string(port),
        version
    );
}

#if defined(MQTT_USE_WS)

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
make_tls_async_client_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, as::io_context::strand>, 4>>>
make_tls_async_client_ws_32(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_ws_32(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
make_tls_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::string port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    using async_client_t = async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>;
    return std::make_shared<callable_overlay<async_client_t>>(
        async_client_t::constructor_access(),
        ioc,
        force_move(host),
        force_move(port),
        force_move(path),
        version
    );
}

inline std::shared_ptr<callable_overlay<async_client<ws_endpoint<tls::stream<as::ip::tcp::socket>, null_strand>, 4>>>
make_tls_async_client_no_strand_ws_32(as::io_context& ioc, std::string host, std::uint16_t port, std::string path = "/", protocol_version version = protocol_version::v3_1_1) {
    return make_tls_async_client_no_strand_ws_32(
        ioc,
        force_move(host),
        std::to_string(port),
        force_move(path),
        version
    );
}

#endif // defined(MQTT_USE_WS)

#endif // defined(MQTT_USE_TLS)

} // namespace MQTT_NS

#endif // MQTT_ASYNC_CLIENT_HPP
