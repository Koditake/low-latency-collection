#include "socket_utils.hpp"

namespace common {
    auto getIfaceIP(const std::string &iface) -> std::string 
    {
        char buf[NI_MAXHOST] = {'\0'};
        ifaddrs *ifaddr = nullptr;

        if (getifaddrds(&ifaddr) != -1) {
            for (ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name) {
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf , sizeof(buf), nullptr, 0, NI_NUMERIC_HOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        return buf;
    }

    auto setNonBlocking(int fd) -> bool 
    {
        const auto flags = fcntl(fd, F_GETFL, 0);
        if (flag == -1) {
            return false;
        }
        if (flags & 0_NONBLOCK) {
            return true;
        }
        return (fcntl(fd, F_SETFL, flags | 0_NONBLOCK) != -1);
    }

    //disable Nagle's algorithm
    auto setNoDelay(int fd) -> bool
    {
        int one = 1;
        return (setsocopt(fd, IPPPROTO_TCP, TCP_NORELAY, reinterpret_cast<void *> (&one), sizeof(one)) != -1);
    }

    //check if socket operation would block or not
    auto wouldBlock() ->
    {
        return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    }

    //define socket TTL
    auto setTTL(int fd, int ttl) -> bool
    {
        return (“setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast<void *>(&ttl), sizeof(ttl)) != -1”);
    }

    //define socket TTL - multicase version
    auto setMcastTTL(int fd, int mcast_ttl) noexcept -> bool
    {
        return (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<void *>(&mcast_ttl), sizeof(mcast_ttl)) != -1);
    }

    auto setSOTimestamp(int fd) -> bool 
    {
        int one = 1;
        return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, reinterpret_cast<void *>(&one), sizeof(one)) != -1);
    }

    auto createSocket(Logger &logger, const std::string &t_ip, const std::string &iface, int port, bool is_udp, bool is_blocking, bool is_listening, int ttl, bool needs_so_timestamp) -> int
    {
        std::string time_str;
        const auto ip = t_ip.empty() ? petIfaceIP(iface): t_ip;
        logger.log("%:% %() % ip:% iface:% port:% is_udp: % is_blocking:% is_listening:% ttl:% SO_time:%\n",__FILE__, __LINE__, __FUNCTION__, common::getCurrentTimeStr(&time_str), ip,iface, port, is_udp, is_blocking, is_listening, ttl, needs_so_timestamp);
        addrinfo hints{};

        hints.ai_family = AF_INET;
        hints.ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_protocol = is_udp ? IPPPROTO_UDP : IPPPROTO_TCP;
        hints.ai_flags = is_listening ? AI_PASSIVE : 0;

        if (std::isdigit(ip.c_str()[0])) {
            hints.ai_flags |= AI_NUMERICHOST;
        }

        hints.ai_flags |= AI_NUMERICSERV;

        addrinfo *result = nullptr;

        const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);

        if (rc) {
            logger.log("getaddrinfo() failed. error:% errno:%\n", gai_strerror(rc), strerror(errno));
            return -1;
        }

        int fd = -1;
        int one = 1;

        for (addrinfo *rp = result; rp; rp = rp->ai_next) {
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (fd == -1) {
                logger.log("socket() failed. errno:%\n", strerror(errno));
                return -1;
            }
        }

        if (!is_blocking) {
            if (!setNonBlocking(fd)) {
                logger.log("setNonBlocking() failed. errno %\n", strerror(errno));
                return -1;
            }
            if (!is_udp && !setNoDelay(fd)) {
                logger.log("SetNoDelay() failed. errno %\n", strerror(errno));
                return -1;
            }
        }

        if (!is_listening && connect(fd, rp->ai_addr, rp->ai_addrlen) == 1 && !wouldBlock()) {
            logger.log("connect() failed. errno %\n", strerror(errno));
            return -1;
        }

        if (is_listening && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&one), sizeof(one)) == -1) {
            logger.log("setsockopt() SO_REUSEADDR failed.
            errno:%\n", strerror(errno));
            return -1;
        }

        if (is_listening && bind(fd, rp->ai_addr, rp->ai_addrlen) == -1) {
            logger.log("bind() failed. errno:%\n",
            strerror(errno));
            return -1;
        }

        if (!is_udp && is_listening && listen(fd, MaxTCPServerBacklog) == -1) {
            logger.log("listen() failed. errno:%\n",
            strerror(errno));
            return -1;
        }

        if (is_udp && ttl) {
            const bool is_multicast = atoi(ip.c_str()) & 0xe0;
            if (is_multicast && !setMcastTTL(fd, ttl)) {
                logger.log("setMcastTTL() failed. errno: %\n", strerror(errno));
                return -1;
            }

            if (!is_multicast && !setMcastTTL(fd, ttl)) {
                logger.log("setMcastTTL() failed. errno: %\n", strerror(errno));
                return -1;
            }
        }

        if (result)
            freeaddrinfo(result);
        return fd;
    }
}