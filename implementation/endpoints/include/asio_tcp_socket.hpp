// Copyright (C) 2025 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef VSOMEIP_V3_ASIO_TCP_SOCKET_HPP_
#define VSOMEIP_V3_ASIO_TCP_SOCKET_HPP_

#include "tcp_socket.hpp"

#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>

namespace vsomeip_v3 {

class asio_tcp_acceptor;

class asio_tcp_socket final : public tcp_socket {
public:
    asio_tcp_socket(boost::asio::io_context& _io)
        : socket_(_io), ssl_stream_(), ssl_ctx_(nullptr), use_tls_(false) { }

private:
    [[nodiscard]] bool is_open() const override { return socket_.is_open(); }
    [[nodiscard]] int native_handle() override { return socket_.native_handle(); }

    void open(boost::asio::ip::tcp::endpoint::protocol_type pt, boost::system::error_code& ec) override { socket_.open(pt, ec); }
    void bind(boost::asio::ip::tcp::endpoint const& ep, boost::system::error_code& ec) override { socket_.bind(ep, ec); }

    void close(boost::system::error_code& ec) override { socket_.close(ec); }
    void cancel(boost::system::error_code& ec) override { socket_.cancel(ec); }
    void shutdown(boost::asio::ip::tcp::socket::shutdown_type st, boost::system::error_code& ec) override { socket_.shutdown(st, ec); }
    void set_option(boost::asio::ip::tcp::no_delay nd, boost::system::error_code& ec) override { socket_.set_option(nd, ec); }
    void set_option(boost::asio::ip::tcp::socket::keep_alive ka, boost::system::error_code& ec) override { socket_.set_option(ka, ec); }
    void set_option(boost::asio::ip::tcp::socket::linger l, boost::system::error_code& ec) override { socket_.set_option(l, ec); }
    void set_option(boost::asio::ip::tcp::socket::reuse_address ra, boost::system::error_code& ec) override { socket_.set_option(ra, ec); }
#if defined(__linux__) || defined(ANDROID)
    [[nodiscard]] bool set_user_timeout(unsigned int timeout) override {
        return setsockopt(socket_.native_handle(), IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout)) != -1;
    }
#endif
#if defined(__linux__) || defined(ANDROID) || defined(__QNX__)
    [[nodiscard]] bool bind_to_device(std::string const& _device) override {
        return setsockopt(socket_.native_handle(), SOL_SOCKET, SO_BINDTODEVICE, _device.c_str(), static_cast<socklen_t>(_device.size()))
                != -1;
    }
    [[nodiscard]] bool can_read_fd_flags() override { return fcntl(socket_.native_handle(), F_GETFD) != -1; }
#endif
    boost::asio::ip::tcp::endpoint local_endpoint(boost::system::error_code& ec) const override { return socket_.local_endpoint(ec); }
    boost::asio::ip::tcp::endpoint remote_endpoint(boost::system::error_code& ec) const override { return socket_.remote_endpoint(ec); }
    void async_connect(boost::asio::ip::tcp::endpoint const& ep, connect_handler handler) override {
        socket_.async_connect(ep, std::move(handler));
    }
    void async_receive(boost::asio::mutable_buffer b, rw_handler handler) override {
        if (use_tls_ && ssl_stream_) {
            ssl_stream_->async_read_some(b, std::move(handler));
        } else {
            socket_.async_receive(b, std::move(handler));
        }
    }
    void async_write(std::vector<boost::asio::const_buffer> const& bs, rw_handler handler) override {
        if (use_tls_ && ssl_stream_) {
            boost::asio::async_write(*ssl_stream_, bs, std::move(handler));
        } else {
            boost::asio::async_write(socket_, bs, std::move(handler));
        }
    }
    void async_write(boost::asio::const_buffer const& b, completion_condition cc, rw_handler handler) override {
        if (use_tls_ && ssl_stream_) {
            boost::asio::async_write(*ssl_stream_, b, std::move(cc), std::move(handler));
        } else {
            boost::asio::async_write(socket_, b, std::move(cc), std::move(handler));
        }
    }

    bool set_tls_context(std::shared_ptr<void> ctx) override {
        ssl_ctx_ = std::static_pointer_cast<boost::asio::ssl::context>(ctx);
        if (!ssl_ctx_) return false;
        ssl_stream_ = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>(socket_, *ssl_ctx_);
        use_tls_ = true;
        return true;
    }
    bool handshake_client() override {
        if (!use_tls_ || !ssl_stream_) return false;
        boost::system::error_code ec;
        ssl_stream_->handshake(boost::asio::ssl::stream_base::client, ec);
        return !ec;
    }
    bool handshake_server() override {
        if (!use_tls_ || !ssl_stream_) return false;
        boost::system::error_code ec;
        ssl_stream_->handshake(boost::asio::ssl::stream_base::server, ec);
        return !ec;
    }

    // needs to access the socket member to create a meaningful new connection
    friend class asio_tcp_acceptor;
    boost::asio::ip::tcp::socket socket_;
    std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> ssl_stream_;
    std::shared_ptr<boost::asio::ssl::context> ssl_ctx_;
    bool use_tls_;
};

class asio_tcp_acceptor final : public tcp_acceptor {
public:
    asio_tcp_acceptor(boost::asio::io_context& _io) : acceptor_(_io) { }

private:
    [[nodiscard]] bool is_open() const override { return acceptor_.is_open(); }
    [[nodiscard]] int native_handle() override { return acceptor_.native_handle(); }

    void open(boost::asio::ip::tcp::endpoint::protocol_type pt, boost::system::error_code& ec) override { acceptor_.open(pt, ec); }
    void bind(boost::asio::ip::tcp::endpoint const& ep, boost::system::error_code& ec) override { acceptor_.bind(ep, ec); }
    void close(boost::system::error_code& ec) override { acceptor_.close(ec); }
    void listen(int backlog, boost::system::error_code& ec) override { acceptor_.listen(backlog, ec); }

    void set_option(boost::asio::ip::tcp::socket::reuse_address ra, boost::system::error_code& ec) override {
        acceptor_.set_option(ra, ec);
    }

#if defined(__linux__) || defined(ANDROID)
    [[nodiscard]] bool set_native_option_free_bind() override {
        int opt = 1;
        return setsockopt(acceptor_.native_handle(), IPPROTO_IP, IP_FREEBIND, &opt, sizeof(opt)) == 0;
    }
#endif

    void async_accept(tcp_socket& socket, connect_handler handler) override {
        auto* socket_impl = dynamic_cast<asio_tcp_socket*>(&socket);
        if (!socket_impl) {
            handler(boost::system::errc::make_error_code(boost::system::errc::invalid_argument));
            return;
        }
        acceptor_.async_accept(socket_impl->socket_, std::move(handler));
    }

    boost::asio::ip::tcp::acceptor acceptor_;
};

}
#endif
