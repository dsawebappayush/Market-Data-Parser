  #include <cstdio>
  #include <cstdlib>
  #include <cstddef>
  #include <vector>
  #include <span>
  #include <chrono>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>

  #include "parser.hpp"

  struct CountingHandler {
      std::size_t adds = 0, execs = 0, deletes = 0;
      void on_add(const AddOrderMessage&)          noexcept { ++adds; }
      void on_executed(const OrderExecutedMessage&) noexcept { ++execs; }
      void on_delete(const OrderDeleteMessage&)     noexcept { ++deletes; }
  };

  int main(int argc, char** argv) {
      if (argc != 2) { std::fprintf(stderr, "usage: %s <port>\n", argv[0]); return 1; }
      const int port = std::atoi(argv[1]);

      // 1. create a UDP socket
      const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
      if (sock < 0) { std::perror("socket"); return 1; }

      // 2. big receive buffer so bursts don't get dropped by the kernel
      int rcvbuf = 64 * 1024 * 1024;
      ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

      // 3. bind to the port on all local interfaces
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port = htons(static_cast<uint16_t>(port));
      if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
          std::perror("bind"); ::close(sock); return 1;
      }
      std::printf("listening on UDP %d ...\n", port);

      CountingHandler h;
      unsigned long long parse_ns = 0;           // total nanoseconds spent in parse
      std::size_t parsed_bytes = 0;              // total bytes handed to parse
      std::vector<std::byte> carry;           // leftover partial message from last packet
      std::vector<std::byte> pkt(65536);      // scratch buffer for one datagram

      for (;;) {
          const ssize_t n = ::recv(sock, pkt.data(), pkt.size(), 0);
          if (n < 0) { std::perror("recv"); break; }
          if (n == 0) break;                  // zero-length datagram = EOF sentinel

          // append new bytes to whatever partial message we carried over
          carry.insert(carry.end(), pkt.begin(), pkt.begin() + n);

          // parse all COMPLETE messages; keep the partial tail for next time
          auto t0 = std::chrono::steady_clock::now();
          const std::size_t consumed =
              parse_partial(std::span<const std::byte>{carry.data(), carry.size()}, h);
          auto t1 = std::chrono::steady_clock::now();
          parse_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
          parsed_bytes += consumed;
          carry.erase(carry.begin(), carry.begin() + consumed);
      }

      double secs = static_cast<double>(parse_ns) / 1e9;
      double gb_sec = secs > 0.0 ? (static_cast<double>(parsed_bytes) / 1e9) / secs : 0.0;
      std::printf("parse time : %.6f s\n", secs);
      std::printf("parsed bytes: %zu\n", parsed_bytes);
      std::printf("GB / s     : %.2f\n", gb_sec);

      std::printf("received over UDP:\n  A: %zu\n  E: %zu\n  D: %zu\n", h.adds, h.execs, h.deletes);
      ::close(sock);
      return 0;
  }
