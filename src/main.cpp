  #include <cstdio>
  #include <cstdlib>
  #include <cstddef>
  #include <span>

  #include <fcntl.h>      // open
  #include <sys/mman.h>   // mmap, madvise
  #include <sys/stat.h>   // fstat
  #include <unistd.h>     // close

  #include "parser.hpp"

  // A minimal Handler: just tallies each message type. The parser only requires
  // on_add / on_executed / on_delete -- swap this for an OrderBook later and the
  // parser code never changes.
  using namespace std;
  struct CountingHandler {
      std::size_t adds = 0;
      std::size_t execs = 0;
      std::size_t deletes = 0;

      void on_add(const AddOrderMessage&)          noexcept { ++adds; }
      void on_executed(const OrderExecutedMessage&) noexcept { ++execs; }
      void on_delete(const OrderDeleteMessage&)     noexcept { ++deletes; }
  };

  int main(int argc, char** argv) {
      if (argc != 2) {
          std::fprintf(stderr, "usage: %s <itch_file>\n", argv[0]);
          return EXIT_FAILURE;
      }

      // 1. open the file read-only
      const int fd = ::open(argv[1], O_RDONLY);
      if (fd < 0) { std::perror("open"); return EXIT_FAILURE; }

      // 2. find its size
      struct stat st{};
      if (::fstat(fd, &st) != 0) { std::perror("fstat"); ::close(fd); return EXIT_FAILURE; }
      const auto size = static_cast<std::size_t>(st.st_size);

      // 3. map the whole file into memory -- zero-copy input
      void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (addr == MAP_FAILED) { std::perror("mmap"); ::close(fd); return EXIT_FAILURE; }
      ::madvise(addr, size, MADV_SEQUENTIAL);   // hint: we read front-to-back

      // 4. hand the bytes to the parser as a span
      std::span<const std::byte> buf{static_cast<const std::byte*>(addr), size};

      CountingHandler handler;
      const std::size_t n = parse(buf, handler);

      // 5. report
      std::printf("dispatched: %zu\n", n);
      std::printf("  add     (A): %zu\n", handler.adds);
      std::printf("  execute (E): %zu\n", handler.execs);
      std::printf("  delete  (D): %zu\n", handler.deletes);

      // 6. clean up
      ::munmap(addr, size);
      ::close(fd);
      return EXIT_SUCCESS;
  }
