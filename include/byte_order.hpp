  #pragma once
  #include <cstdint>
  #include <cstring>

  // ITCH is big-endian (network byte order). x86 is little-endian, so every
  // multi-byte integer must be swapped when read. These helpers pull a big-endian
  // integer out of a raw byte pointer with no alignment assumptions (memcpy is
  // the portable, UB-free way to do a typed load from arbitrary bytes; the
  // compiler folds it into a single mov).

  // Host-endian detection without <bit> (so this builds on C++20 and older GCC).
  #if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
      #define ITCH_HOST_LITTLE 1
  #else
      #define ITCH_HOST_LITTLE 0
  #endif

  inline uint16_t load_be16(const void* p) noexcept {
      uint16_t v;
      std::memcpy(&v, p, sizeof(v));
  #if ITCH_HOST_LITTLE
      return __builtin_bswap16(v);
  #else
      return v;
  #endif
  }

  inline uint32_t load_be32(const void* p) noexcept {
      uint32_t v;
      std::memcpy(&v, p, sizeof(v));
  #if ITCH_HOST_LITTLE
      return __builtin_bswap32(v);
  #else
      return v;
  #endif
  }

  inline uint64_t load_be64(const void* p) noexcept {
      uint64_t v;
      std::memcpy(&v, p, sizeof(v));
  #if ITCH_HOST_LITTLE
      return __builtin_bswap64(v);
  #else
      return v;
  #endif
  }

  // ITCH timestamps are 48-bit (6 bytes) nanoseconds since midnight. There is no
  // native 6-byte integer, so assemble it from the six big-endian bytes directly.
  // (This is endian-agnostic: it reads bytes in wire order regardless of host.)
  inline uint64_t load_be48(const void* p) noexcept {
      const auto* b = static_cast<const uint8_t*>(p);
      return (static_cast<uint64_t>(b[0]) << 40) |
             (static_cast<uint64_t>(b[1]) << 32) |
             (static_cast<uint64_t>(b[2]) << 24) |
             (static_cast<uint64_t>(b[3]) << 16) |
             (static_cast<uint64_t>(b[4]) << 8)  |
             (static_cast<uint64_t>(b[5]));
  }