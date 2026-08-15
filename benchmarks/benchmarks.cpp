  #include <cstdio>
  #include <cstddef>
  #include <span>
  #include <chrono>
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>

  #include "parser.hpp"

  // Compiler barriers: stop -O3 from deleting or hoisting the timed work.
  // do_not_optimize() forces a value to be "used"; clobber() invalidates the
  // compiler's assumptions about memory so mmap reads can't be cached away.
  template <class T> inline void do_not_optimize(const T& v) {
      asm volatile("" : : "r,m"(v) : "memory");
  }
  inline void clobber() { asm volatile("" : : : "memory"); }

  struct BenchHandler {
      std::size_t n = 0;
      void on_add(const AddOrderMessage&)          noexcept { ++n; }
      void on_executed(const OrderExecutedMessage&) noexcept { ++n; }
      void on_delete(const OrderDeleteMessage&)     noexcept { ++n; }
  };

  int main(int argc, char** argv) {
      if (argc != 2) { std::fprintf(stderr, "usage: %s <itch_file>\n", argv[0]); return 1; }

      int fd = ::open(argv[1], O_RDONLY);
      struct stat st{}; ::fstat(fd, &st);
      auto size = static_cast<std::size_t>(st.st_size);
      void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      ::madvise(addr, size, MADV_WILLNEED);
      std::span<const std::byte> buf{static_cast<const std::byte*>(addr), size};

      // Warm-up: fault all pages into RAM so we time CPU, not disk.
      volatile std::byte sink{};
      for (std::size_t i = 0; i < size; i += 4096) sink = buf[i];

      constexpr int RUNS = 5;
      std::size_t msgs = 0;
      double best_ns = 1e30;

      for (int r = 0; r < RUNS; ++r) {
          BenchHandler h;
          do_not_optimize(buf.data());              // buf is "unknown" to optimizer
          auto t0 = std::chrono::steady_clock::now();
          parse(buf, h);
          do_not_optimize(h.n);                      // result must be kept
          clobber();                                 // memory effects must happen here
          auto t1 = std::chrono::steady_clock::now();
          double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
          msgs = h.n;
          if (ns < best_ns) best_ns = ns;
      }

      double secs     = best_ns / 1e9;
      double per_msg  = best_ns / static_cast<double>(msgs);
      double msgs_sec = msgs / secs;
      double gb_sec   = (static_cast<double>(size) / 1e9) / secs;

      std::printf("file size : %.2f MB\n", size / 1e6);
      std::printf("messages  : %zu\n", msgs);
      std::printf("best time : %.3f ms\n", best_ns / 1e6);
      std::printf("ns / msg  : %.2f\n", per_msg);
      std::printf("msgs / s  : %.2f M\n", msgs_sec / 1e6);
      std::printf("GB / s    : %.2f\n", gb_sec);

      ::munmap(addr, size); ::close(fd);
      return 0;
  }