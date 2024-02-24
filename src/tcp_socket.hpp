#pragma once

#include <functional>
#include "socket_utils.hpp"
#include "logger.hpp"


namespace common {
    constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

    struct TCPSocket {
        int fd_ = -1;
        char *send_buffer_ = nullptr;
        size_t next_send_valid_index_ = 0;
        char *rcv_buffer_ = nullptr;
        size_t next_recv_valid_index_ = 0;
        bool send_disconnected_ = false;
        bool recv_disconnected_ = false;
        struct sockaddr_in inInAddr;
        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_;
        std::string time_str_;
        Logger &logger_;

        constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

        auto defaultRecvCallback(TCPSocket *socket, Nanos rx_time) noexcept
        {
            logger_.log("%:% %() % TCPSocket::defaultRecvCallback() socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__, common::getCurrentTimeStr(&time_str_), socket->fd_, socket->next_recv_valid_index_, rx_time
        }

        explicit TCPSocket(Logger &logger): logger_(logger) {
            send_buffer_ = new char[TCPBufferSize];
            rcv_buffer_ = new char[TCPBufferSize];
            //lambda?
            recv_callback_ = [this](auto socket, auto rx_time) {
                defaultRecvCallback(socket, rx_time);
            };
        }

        auto TCPSocket::destroy() noexcept -> void {
            close(fd_);
            fd_ = -1;
        }

        ~TCPSocket() {
            destroy();
            delete[] send_buffer_; send_buffer_ = nullptr;
            delete[] rcv_buffer_; rcv_buffer_ = nullptr;
        }

        TCPSocket() = delete;
        TCPSocket(const TCPSocket &) = delete;
        TCPSocket(const TCPSocket &&) = delete;
        TCPSocket &operator=(const TCPSocket &) = delete;
        TCPSocket &operator=(const TCPSocket &&) = delete;

        auto TCPSocket::connect(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int {
            destroy();
            fd_ = createSocket(logger_, ip, iface, port, false, false, is_listening, 0, true);
            inInAddr.sin_addr.s_addr = INADDR_ANY;
            inInAddr.sin_addr.s_port = htons(port);
            inInAddr.sin_family = AF_INET;
            return fd_;
        }

        auto TCPSocket::send(const void *data, size_t len) noexcept -> void {
            if (len > 0) {
                memcpy(send_buffer_ + next_send_valid_index_, data, len);
                next_send_valid_index_ += len;
            }
        }

        auto TCPSocket::sendAndRecv() noexcept -> bool {
            char ctrl[CMSG_SPACE(sizeof(struct timeval))];
            struct cmsghdr *cmsg = (struct cmsghdr *) &ctrl;
            struct iovec iov;
            iov.iov_base = rcv_buffer_ + next
        }
    }
}