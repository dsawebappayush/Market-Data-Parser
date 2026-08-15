  #pragma once
  #include <cstdint>
  #include <cstddef>
  #include <span>
  #include "itch_messages.hpp"
  #include "byte_order.hpp"

  // The parse loop walks a length-framed ITCH stream and dispatches each message
  // to a user-supplied Handler. The Handler is a TEMPLATE parameter, not a base
  // class -> the compiler inlines the calls, so there are zero virtual-function
  // lookups in the hot loop.
  //
  // A Handler is any type with these three methods:
  //     void on_add(const AddOrderMessage&);
  //     void on_executed(const OrderExecutedMessage&);
  //     void on_delete(const OrderDeleteMessage&);

  // Parse a stream shaped as:  [2-byte BE length][payload] [2-byte BE length][payload] ...
  // Returns how many messages were dispatched to the handler.
  template <typename Handler>
  std::size_t parse(std::span<const std::byte> buf, Handler& h) {
      std::size_t offset = 0;       // where we are in the buffer
      std::size_t dispatched = 0;   // messages handed to the handler

      while (offset + 2 <= buf.size()) {           // need 2 bytes for a length
          const uint16_t len = load_be16(buf.data() + offset);
          if (len == 0) break;                     // 0-length = padding / end of stream
          offset += 2;                             // step past the length field

          if (offset + len > buf.size()) break;    // truncated final record -> stop

          const std::byte* payload = buf.data() + offset;
          const char type = static_cast<char>(payload[0]);

          switch (type) {                          // compiles to a jump table
              case 'A':
                  h.on_add(*reinterpret_cast<const AddOrderMessage*>(payload));
                  ++dispatched;
                  break;
              case 'E':
                  h.on_executed(*reinterpret_cast<const OrderExecutedMessage*>(payload));
                  ++dispatched;
                  break;
              case 'D':
                  h.on_delete(*reinterpret_cast<const OrderDeleteMessage*>(payload));
                  ++dispatched;
                  break;
              default:
                  break;   // unknown type: skip it, framing stays intact via len
          }

          offset += len;   // jump to the next record
      }
      return dispatched;
  }