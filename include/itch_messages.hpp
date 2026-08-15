  #pragma once
  #include <cstdint>
  #include <cstddef>
  #include "byte_order.hpp"   // load_be16/32/48/64 — provided in step 2

  // Zero-copy overlay: these structs map DIRECTLY onto the raw feed bytes.
  // Three rules keep that safe:
  //   1. #pragma pack(1) -> no padding between members.
  //   2. All multi-byte integers are RAW byte arrays, read via load_beNN(),
  //      because ITCH is BIG-ENDIAN and x86 is little-endian.
  //   3. The timestamp is 48-bit = 6 bytes, NOT 8. (This is the #1 ITCH bug:
  //      8 bytes misaligns every field after it.)
  // Never read a raw integer member directly — always use the accessor.

  // Instruct the compiler NOT to pad these structs
  #pragma pack(push, 1)

  // Message Type 'A' - Add Order Message (No MPID). Wire size = 36 bytes.
  struct AddOrderMessage {
      char     message_type;          // 1 byte  - Always 'A'
      uint8_t  stock_locate[2];       // 2 bytes - Ticker index
      uint8_t  tracking_number[2];    // 2 bytes - Internal system tracking
      uint8_t  timestamp[6];          // 6 bytes - Nanoseconds since midnight (48-bit!)
      uint8_t  order_reference[8];    // 8 bytes - Unique ID of the order
      char     buy_sell_indicator;    // 1 byte  - 'B' (Buy) or 'S' (Sell)
      uint8_t  shares[4];             // 4 bytes - Number of shares
      char     stock[8];              // 8 bytes - Ticker symbol (e.g., "AAPL    ")
      uint8_t  price[4];              // 4 bytes - Price * 10,000

      uint16_t stock_locate_() const noexcept { return load_be16(stock_locate); }
      uint16_t tracking_()     const noexcept { return load_be16(tracking_number); }
      uint64_t timestamp_()    const noexcept { return load_be48(timestamp); }
      uint64_t order_ref_()    const noexcept { return load_be64(order_reference); }
      uint32_t shares_()       const noexcept { return load_be32(shares); }
      uint32_t price_()        const noexcept { return load_be32(price); }
  };
  static_assert(sizeof(AddOrderMessage) == 36, "AddOrderMessage must be 36 bytes");

  // Message Type 'E' - Order Executed Message. Wire size = 31 bytes.
  struct OrderExecutedMessage {
      char     message_type;          // 1 byte  - Always 'E'
      uint8_t  stock_locate[2];       // 2 bytes
      uint8_t  tracking_number[2];    // 2 bytes
      uint8_t  timestamp[6];          // 6 bytes
      uint8_t  order_reference[8];    // 8 bytes - ID of the order being executed
      uint8_t  executed_shares[4];    // 4 bytes - Number of shares executed
      uint8_t  match_number[8];       // 8 bytes - Unique match ID

      uint16_t stock_locate_()    const noexcept { return load_be16(stock_locate); }
      uint64_t timestamp_()       const noexcept { return load_be48(timestamp); }
      uint64_t order_ref_()       const noexcept { return load_be64(order_reference); }
      uint32_t executed_shares_() const noexcept { return load_be32(executed_shares); }
      uint64_t match_number_()    const noexcept { return load_be64(match_number); }
  };
  static_assert(sizeof(OrderExecutedMessage) == 31, "OrderExecutedMessage must be 31 bytes");

  // Message Type 'D' - Order Delete Message. Wire size = 19 bytes.
  struct OrderDeleteMessage {
      char     message_type;          // 1 byte  - Always 'D'
      uint8_t  stock_locate[2];       // 2 bytes
      uint8_t  tracking_number[2];    // 2 bytes
      uint8_t  timestamp[6];          // 6 bytes
      uint8_t  order_reference[8];    // 8 bytes

      uint16_t stock_locate_() const noexcept { return load_be16(stock_locate); }
      uint64_t timestamp_()    const noexcept { return load_be48(timestamp); }
      uint64_t order_ref_()    const noexcept { return load_be64(order_reference); }
  };
  static_assert(sizeof(OrderDeleteMessage) == 19, "OrderDeleteMessage must be 19 bytes");

  // Restore default compiler padding behavior for the rest of the project
  #pragma pack(pop)