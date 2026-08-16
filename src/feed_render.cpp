
  #include <cstdio>
  #include <cstdlib>
  #include <cstddef>
  #include <algorithm>
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>

  int main(int argc, char** argv) {
      if (argc != 4) { std::fprintf(stderr, "usage: %s <file> <ip> <port>\n", argv[0]); return 1; }
      const char* path = argv[1];
      const char* ip   = argv[2];
      const int   port = std::atoi(argv[3]);

      // 1. mmap the file (same zero-copy input as before)
      const int fd = ::open(path, O_RDONLY);
      if (fd < 0) { std::perror("open"); return 1; }
      struct stat st{}; ::fstat(fd, &st);
      const auto size = static_cast<std::size_t>(st.st_size);
      const auto* base = static_cast<const unsigned char*>(
          ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));

      // 2. set up the destination UDP socket
      const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
      sockaddr_in dst{};
      dst.sin_family = AF_INET;
      dst.sin_port   = htons(static_cast<uint16_t>(port));
      ::inet_pton(AF_INET, ip, &dst.sin_addr);

      // 3. blast the file out in MTU-sized chunks
      constexpr std::size_t CHUNK = 1400;     // safe UDP payload (< typical MTU)
      std::size_t packets = 0;
      for (std::size_t off = 0; off < size; off += CHUNK) {
          const std::size_t len = std::min(CHUNK, size - off);
          ::sendto(sock, base + off, len, 0,
                   reinterpret_cast<sockaddr*>(&dst), sizeof(dst));
          if ((++packets & 0x3FF) == 0) ::usleep(50);  // gentle throttle so recv keeps up
      }

      // 4. zero-length datagram tells the receiver we're done
      ::sendto(sock, base, 0, 0, reinterpret_cast<sockaddr*>(&dst), sizeof(dst));

      std::printf("sent %zu packets (%.2f MB)\n", packets, size / 1e6);
      ::munmap(const_cast<unsigned char*>(base), size);
      ::close(sock); ::close(fd);
      return 0;
  }