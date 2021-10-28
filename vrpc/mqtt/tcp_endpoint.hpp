// Copyright Takatoshi Kondo 2017
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_TCP_ENDPOINT_HPP)
#define MQTT_TCP_ENDPOINT_HPP

#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>

#include <vrpc/mqtt/namespace.hpp>
#include <vrpc/mqtt/type_erased_socket.hpp>
#include <vrpc/mqtt/move.hpp>
#include <vrpc/mqtt/attributes.hpp>

namespace MQTT_NS {

namespace as = boost::asio;

template <typename Socket, typename Strand>
class tcp_endpoint : public socket {
public:
    template <typename... Args>
    tcp_endpoint(as::io_context& ioc, Args&&... args)
        :tcp_(ioc, std::forward<Args>(args)...),
         strand_(ioc) {
    }

    MQTT_ALWAYS_INLINE void async_read(
        as::mutable_buffer buffers,
        std::function<void(error_code, std::size_t)> handler
    ) override final {
        as::async_read(
            tcp_,
            force_move(buffers),
            as::bind_executor(
                strand_,
                force_move(handler)
            )
        );
    }

    MQTT_ALWAYS_INLINE void async_write(
        std::vector<as::const_buffer> buffers,
        std::function<void(error_code, std::size_t)> handler
    ) override final {
        as::async_write(
            tcp_,
            force_move(buffers),
            as::bind_executor(
                strand_,
                force_move(handler)
            )
        );
    }

    MQTT_ALWAYS_INLINE std::size_t write(
        std::vector<as::const_buffer> buffers,
        boost::system::error_code& ec
    ) override final {
        return as::write(tcp_,force_move(buffers), ec);
    }

    MQTT_ALWAYS_INLINE void post(std::function<void()> handler) override final {
        as::post(
            strand_,
            force_move(handler)
        );
    }

    MQTT_ALWAYS_INLINE as::ip::tcp::socket::lowest_layer_type& lowest_layer() override final {
        return tcp_.lowest_layer();
    }

    MQTT_ALWAYS_INLINE any native_handle() override final {
        return tcp_.native_handle();
    }

    MQTT_ALWAYS_INLINE void close(boost::system::error_code& ec) override final {
        tcp_.lowest_layer().close(ec);
    }

#if BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
    MQTT_ALWAYS_INLINE as::executor get_executor() override final {
        return lowest_layer().get_executor();
    }
#else  // BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
    MQTT_ALWAYS_INLINE as::any_io_executor get_executor() override final {
        return lowest_layer().get_executor();
    }
#endif // BOOST_VERSION < 107400 || defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

    auto& socket() { return tcp_; }
    auto const& socket() const { return tcp_; }

    template <typename... Args>
    void set_option(Args&& ... args) {
        tcp_.set_option(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void async_accept(Args&& ... args) {
        tcp_.async_accept(std::forward<Args>(args)...);
    }

#if defined(MQTT_USE_TLS)

    template <typename... Args>
    void handshake(Args&& ... args) {
        tcp_.handshake(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void async_handshake(Args&& ... args) {
        tcp_.async_handshake(std::forward<Args>(args)...);
    }

#endif // defined(MQTT_USE_TLS)

private:
    Socket tcp_;
    Strand strand_;
};

} // namespace MQTT_NS

#endif // MQTT_TCP_ENDPOINT_HPP
