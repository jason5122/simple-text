// Glaze Library
// For the license information refer to glaze.hpp

#pragma once

#include <bit>
#include <cstring>
#include <iterator>
#include <span>

#include "glaze/core/context.hpp"
#include "glaze/core/meta.hpp"
#include "glaze/core/opts.hpp"
#include "glaze/util/expected.hpp"
#include "glaze/util/inline.hpp"
#include "glaze/util/stoui64.hpp"
#include "glaze/util/string_literal.hpp"

namespace glz::detail
{
   constexpr std::array<bool, 256> numeric_table = [] {
      std::array<bool, 256> t{};
      t['0'] = true;
      t['1'] = true;
      t['2'] = true;
      t['3'] = true;
      t['4'] = true;
      t['5'] = true;
      t['6'] = true;
      t['7'] = true;
      t['8'] = true;
      t['9'] = true;
      t['.'] = true;
      t['+'] = true;
      t['-'] = true;
      t['e'] = true;
      t['E'] = true;
      return t;
   }();

   constexpr std::array<char, 256> char_unescape_table = [] {
      std::array<char, 256> t{};
      t['"'] = '"';
      t['/'] = '/';
      t['\\'] = '\\';
      t['b'] = '\b';
      t['f'] = '\f';
      t['n'] = '\n';
      t['r'] = '\r';
      t['t'] = '\t';
      return t;
   }();

   constexpr std::array<bool, 256> whitespace_table = [] {
      std::array<bool, 256> t{};
      t['\n'] = true;
      t['\t'] = true;
      t['\r'] = true;
      t[' '] = true;
      return t;
   }();

   constexpr std::array<bool, 256> whitespace_comment_table = [] {
      std::array<bool, 256> t{};
      t['\n'] = true;
      t['\t'] = true;
      t['\r'] = true;
      t[' '] = true;
      t['/'] = true;
      return t;
   }();

   constexpr std::array<uint8_t, 256> digit_hex_table = [] {
      std::array<uint8_t, 256> t;
      std::fill(t.begin(), t.end(), uint8_t(255));
      t['0'] = 0;
      t['1'] = 1;
      t['2'] = 2;
      t['3'] = 3;
      t['4'] = 4;
      t['5'] = 5;
      t['6'] = 6;
      t['7'] = 7;
      t['8'] = 8;
      t['9'] = 9;
      t['a'] = 0xA;
      t['b'] = 0xB;
      t['c'] = 0xC;
      t['d'] = 0xD;
      t['e'] = 0xE;
      t['f'] = 0xF;
      t['A'] = 0xA;
      t['B'] = 0xB;
      t['C'] = 0xC;
      t['D'] = 0xD;
      t['E'] = 0xE;
      t['F'] = 0xF;
      return t;
   }();

   template <class T>
   consteval uint32_t repeat_byte4(const T repeat)
   {
      const auto byte = uint8_t(repeat);
      uint32_t res{};
      res |= uint32_t(byte) << 24;
      res |= uint32_t(byte) << 16;
      res |= uint32_t(byte) << 8;
      res |= uint32_t(byte);
      return res;
   }

   GLZ_ALWAYS_INLINE constexpr uint32_t has_zero_u32(const uint32_t chunk) noexcept
   {
      return (((chunk - 0x01010101u) & ~chunk) & 0x80808080u);
   }

   GLZ_ALWAYS_INLINE constexpr uint32_t is_less_16_u32(const uint32_t chunk) noexcept
   {
      return has_zero_u32(chunk & repeat_byte4(0b11110000u));
   }

   template <class T>
   consteval uint64_t repeat_byte8(const T repeat)
   {
      const auto byte = uint8_t(repeat);
      uint64_t res{};
      res |= uint64_t(byte) << 56;
      res |= uint64_t(byte) << 48;
      res |= uint64_t(byte) << 40;
      res |= uint64_t(byte) << 32;
      res |= uint64_t(byte) << 24;
      res |= uint64_t(byte) << 16;
      res |= uint64_t(byte) << 8;
      res |= uint64_t(byte);
      return res;
   }

   [[nodiscard]] GLZ_ALWAYS_INLINE uint32_t hex_to_u32(const char* c) noexcept
   {
      const auto& t = digit_hex_table;
      const uint8_t arr[4]{t[c[3]], t[c[2]], t[c[1]], t[c[0]]};
      uint32_t chunk;
      std::memcpy(&chunk, arr, 4);
      // check that all hex characters are valid
      if (is_less_16_u32(chunk)) [[likely]] {
         // now pack into first four bytes of uint32_t
         uint32_t packed{};
         packed |= (chunk & 0x0000000F);
         packed |= (chunk & 0x00000F00) >> 4;
         packed |= (chunk & 0x000F0000) >> 8;
         packed |= (chunk & 0x0F000000) >> 12;
         return packed;
      }
      return 0xFFFFFFFFu;
   }

   template <class Char>
   [[nodiscard]] GLZ_ALWAYS_INLINE uint32_t code_point_to_utf8(const uint32_t code_point, Char* c)
   {
      if (code_point <= 0x7F) {
         c[0] = Char(code_point);
         return 1;
      }
      if (code_point <= 0x7FF) {
         c[0] = Char(0xC0 | ((code_point >> 6) & 0x1F));
         c[1] = Char(0x80 | (code_point & 0x3F));
         return 2;
      }
      if (code_point <= 0xFFFF) {
         c[0] = Char(0xE0 | ((code_point >> 12) & 0x0F));
         c[1] = Char(0x80 | ((code_point >> 6) & 0x3F));
         c[2] = Char(0x80 | (code_point & 0x3F));
         return 3;
      }
      if (code_point <= 0x10FFFF) {
         c[0] = Char(0xF0 | ((code_point >> 18) & 0x07));
         c[1] = Char(0x80 | ((code_point >> 12) & 0x3F));
         c[2] = Char(0x80 | ((code_point >> 6) & 0x3F));
         c[3] = Char(0x80 | (code_point & 0x3F));
         return 4;
      }
      return 0;
   }

   [[nodiscard]] GLZ_ALWAYS_INLINE uint32_t skip_code_point(const uint32_t code_point)
   {
      if (code_point <= 0x7F) {
         return 1;
      }
      if (code_point <= 0x7FF) {
         return 2;
      }
      if (code_point <= 0xFFFF) {
         return 3;
      }
      if (code_point <= 0x10FFFF) {
         return 4;
      }
      return 0;
   }

   namespace unicode
   {
      constexpr uint32_t generic_surrogate_mask = 0xF800;
      constexpr uint32_t generic_surrogate_value = 0xD800;

      constexpr uint32_t surrogate_mask = 0xFC00;
      constexpr uint32_t high_surrogate_value = 0xD800;
      constexpr uint32_t low_surrogate_value = 0xDC00;

      constexpr uint32_t surrogate_codepoint_offset = 0x10000;
      constexpr uint32_t surrogate_codepoint_mask = 0x03FF;
      constexpr uint32_t surrogate_codepoint_bits = 10;
   }

   template <class Char>
   [[nodiscard]] GLZ_ALWAYS_INLINE bool handle_unicode_code_point(const Char*& it, Char*& dst)
   {
      using namespace unicode;

      const uint32_t high = hex_to_u32(it);
      if (high == 0xFFFFFFFFu) [[unlikely]] {
         return false;
      }
      it += 4; // skip the code point characters

      uint32_t code_point;

      if ((high & generic_surrogate_mask) == generic_surrogate_value) {
         // surrogate pair code points
         if ((high & surrogate_mask) != high_surrogate_value) {
            return false;
         }

         it += 2;
         // verify that second unicode escape sequence is present
         const uint32_t low = hex_to_u32(it);
         if (low == 0xFFFFFFFFu) [[unlikely]] {
            return false;
         }
         it += 4;

         if ((low & surrogate_mask) != low_surrogate_value) {
            return false;
         }

         code_point = (high & surrogate_codepoint_mask) << surrogate_codepoint_bits;
         code_point |= (low & surrogate_codepoint_mask);
         code_point += surrogate_codepoint_offset;
      }
      else {
         code_point = high;
      }
      const uint32_t offset = code_point_to_utf8(code_point, dst);
      dst += offset;
      return offset > 0;
   }

   template <class Char>
   [[nodiscard]] GLZ_ALWAYS_INLINE bool skip_unicode_code_point(const Char*& it, const Char* end)
   {
      using namespace unicode;
      if (it + 4 >= end) [[unlikely]] {
         return false;
      }

      const uint32_t high = hex_to_u32(it);
      if (high == 0xFFFFFFFFu) [[unlikely]] {
         return false;
      }
      it += 4; // skip the code point characters

      uint32_t code_point;

      if ((high & generic_surrogate_mask) == generic_surrogate_value) {
         // surrogate pair code points
         if ((high & surrogate_mask) != high_surrogate_value) {
            return false;
         }

         if (it + 6 >= end) [[unlikely]] {
            return false;
         }
         it += 2;
         // verify that second unicode escape sequence is present
         const uint32_t low = hex_to_u32(it);
         if (low == 0xFFFFFFFFu) [[unlikely]] {
            return false;
         }
         it += 4;

         if ((low & surrogate_mask) != low_surrogate_value) {
            return false;
         }

         code_point = (high & surrogate_codepoint_mask) << surrogate_codepoint_bits;
         code_point |= (low & surrogate_codepoint_mask);
         code_point += surrogate_codepoint_offset;
      }
      else {
         code_point = high;
      }
      return skip_code_point(code_point) > 0;
   }

   // assumes null terminated
   template <char c>
   GLZ_ALWAYS_INLINE void match(is_context auto&& ctx, auto&& it) noexcept
   {
      if (*it != c) [[unlikely]] {
         ctx.error = error_code::syntax_error;
      }
      else [[likely]] {
         ++it;
      }
   }

   // assumes null terminated
   template <char c>
   GLZ_ALWAYS_INLINE void match(is_context auto&& ctx, auto&& it, auto&&) noexcept
   {
      if (*it != c) [[unlikely]] {
         ctx.error = error_code::syntax_error;
      }
      else [[likely]] {
         ++it;
      }
   }

   template <string_literal str>
   GLZ_ALWAYS_INLINE void match(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      const auto n = size_t(end - it);
      if ((n < str.size()) || std::memcmp(&*it, str.value, str.size())) [[unlikely]] {
         ctx.error = error_code::syntax_error;
      }
      else [[likely]] {
         it += str.size();
      }
   }

   GLZ_ALWAYS_INLINE void skip_comment(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      ++it;
      if (it == end) [[unlikely]] {
         ctx.error = error_code::unexpected_end;
      }
      else if (*it == '/') {
         while (++it != end && *it != '\n')
            ;
      }
      else if (*it == '*') {
         while (++it != end) {
            if (*it == '*') [[unlikely]] {
               if (++it == end) [[unlikely]]
                  break;
               else if (*it == '/') [[likely]] {
                  ++it;
                  break;
               }
            }
         }
      }
      else [[unlikely]] {
         ctx.error = error_code::expected_end_comment;
      }
   }

   GLZ_ALWAYS_INLINE constexpr auto has_zero(const uint64_t chunk) noexcept
   {
      return (((chunk - 0x0101010101010101u) & ~chunk) & 0x8080808080808080u);
   }

   GLZ_ALWAYS_INLINE constexpr auto has_quote(const uint64_t chunk) noexcept
   {
      return has_zero(chunk ^ repeat_byte8('"'));
   }

   GLZ_ALWAYS_INLINE constexpr auto has_escape(const uint64_t chunk) noexcept
   {
      return has_zero(chunk ^ repeat_byte8('\\'));
   }

   GLZ_ALWAYS_INLINE constexpr auto has_space(const uint64_t chunk) noexcept
   {
      return has_zero(chunk ^ repeat_byte8(' '));
   }

   template <char Char>
   GLZ_ALWAYS_INLINE constexpr auto has_char(const uint64_t chunk) noexcept
   {
      return has_zero(chunk ^ repeat_byte8(Char));
   }

   GLZ_ALWAYS_INLINE constexpr uint64_t is_less_16(const uint64_t chunk) noexcept
   {
      return has_zero(chunk & repeat_byte8(0b11110000u));
   }

   GLZ_ALWAYS_INLINE constexpr uint64_t is_greater_15(const uint64_t chunk) noexcept
   {
      return (chunk & repeat_byte8(0b11110000u));
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE void skip_ws_no_pre_check(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      if constexpr (!Opts.force_conformance) {
         while (whitespace_comment_table[*it]) {
            if (*it == '/') [[unlikely]] {
               skip_comment(ctx, it, end);
               if (bool(ctx.error)) [[unlikely]] {
                  return;
               }
            }
            else [[likely]] {
               ++it;
            }
         }
      }
      else {
         while (whitespace_table[*it]) {
            ++it;
         }
      }
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE void skip_ws(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      if (bool(ctx.error)) [[unlikely]]
         return;
      skip_ws_no_pre_check<Opts>(ctx, it, end);
   }

   GLZ_ALWAYS_INLINE void skip_matching_ws(const auto* ws, auto&& it, uint64_t length) noexcept
   {
      if (length > 7) {
         uint64_t v[2];
         while (length > 8) {
            std::memcpy(v, ws, 8);
            std::memcpy(v + 1, it, 8);
            if (v[0] != v[1]) {
               return;
            }
            length -= 8;
            ws += 8;
            it += 8;
         }

         const auto shift = 8 - length;
         ws -= shift;
         it -= shift;

         std::memcpy(v, ws, 8);
         std::memcpy(v + 1, it, 8);
         return;
      }
      {
         constexpr uint64_t n{sizeof(uint32_t)};
         if (length >= n) {
            uint32_t v[2];
            std::memcpy(v, ws, n);
            std::memcpy(v + 1, it, n);
            if (v[0] != v[1]) {
               return;
            }
            length -= n;
            ws += n;
            it += n;
         }
      }
      {
         constexpr uint64_t n{sizeof(uint16_t)};
         if (length >= n) {
            uint16_t v[2];
            std::memcpy(v, ws, n);
            std::memcpy(v + 1, it, n);
            if (v[0] != v[1]) {
               return;
            }
            // length -= n;
            // ws += n;
            it += n;
         }
      }
      // We have to call a whitespace check after this function
      // in case the whitespace is mismatching.
      // So, we forgo this check as to not duplicate.
      /*if (length && *ws == *it) {
         ++it;
      }*/
   }

   GLZ_ALWAYS_INLINE void skip_till_escape_or_quote(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

      for (const auto end_m7 = end - 7; it < end_m7; it += 8) {
         uint64_t chunk;
         std::memcpy(&chunk, it, 8);
         const uint64_t test_chars = has_quote(chunk) | has_escape(chunk);
         if (test_chars) {
            it += (std::countr_zero(test_chars) >> 3);
            return;
         }
      }

      // Tail end of buffer. Should be rare we even get here
      while (it < end) {
         switch (*it) {
         case '\\':
         case '"':
            return;
         }
         ++it;
      }

      ctx.error = error_code::expected_quote;
   }

   GLZ_ALWAYS_INLINE void skip_till_quote(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      const auto* pc = std::memchr(it, '"', std::distance(it, end));
      if (pc) [[likely]] {
         it = reinterpret_cast<std::decay_t<decltype(it)>>(pc);
         return;
      }

      ctx.error = error_code::expected_quote;
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE bool skip_till_unescaped_quote(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

      if constexpr (!Opts.force_conformance) {
         bool escaped = false;
         for (const auto fin = end - 7; it < fin;) {
            uint64_t chunk;
            std::memcpy(&chunk, it, 8);
            uint64_t test_chars = has_quote(chunk);
            escaped |= static_cast<bool>(has_escape(chunk));
            if (test_chars) {
               it += (std::countr_zero(test_chars) >> 3);

               auto* prev = it - 1;
               while (*prev == '\\') {
                  --prev;
               }
               if (size_t(it - prev) % 2) {
                  return escaped;
               }
               ++it; // skip the escaped quote
            }
            else {
               it += 8;
            }
         }

         // Tail end of buffer. Should be rare we even get here
         while (it < end) {
            switch (*it) {
            case '\\': {
               escaped = true;
               ++it;
               if (it == end) [[unlikely]] {
                  ctx.error = error_code::expected_quote;
                  return true;
               }
               ++it;
               break;
            }
            case '"': {
               auto* prev = it - 1;
               while (*prev == '\\') {
                  --prev;
                  escaped = true;
               }
               if (size_t(it - prev) % 2) {
                  return escaped;
               }
               ++it; // skip the escaped quote
               break;
            }
            default: {
               ++it;
            }
            }
         }
      }
      else {
         bool escaped = false;
         for (const auto fin = end - 7; it < fin;) {
            uint64_t chunk;
            std::memcpy(&chunk, it, 8);
            uint64_t test_chars = has_quote(chunk) | has_escape(chunk) | is_less_16(chunk);
            if (test_chars) {
               it += (std::countr_zero(test_chars) >> 3);

               if (*it == '"') {
                  return escaped;
               }
               else if (*it == '\\') [[likely]] {
                  ++it;
                  if (char_unescape_table[*it]) [[likely]] {
                     ++it;
                     continue;
                  }
                  else if (*it == 'u') [[unlikely]] {
                     ++it;
                     if (skip_unicode_code_point(it, end)) [[likely]] {
                        continue;
                     }
                     else [[unlikely]] {
                        ctx.error = error_code::unicode_escape_conversion_failure;
                        return true;
                     }
                  }
               }
               ctx.error = error_code::syntax_error;
               return true;
            }
            else {
               it += 8;
            }
         }

         // Tail end of buffer. Should be rare we even get here
         while (it < end) {
            if (*it < 16) [[unlikely]] {
               ctx.error = error_code::syntax_error;
               return true;
            }

            switch (*it) {
            case '\\': {
               escaped = true;
               ++it;
               if (char_unescape_table[*it]) [[likely]] {
                  ++it;
               }
               else if (*it == 'u') {
                  ++it;
                  if (skip_unicode_code_point(it, end)) [[likely]] {
                     continue;
                  }
                  else [[unlikely]] {
                     ctx.error = error_code::unicode_escape_conversion_failure;
                     return true;
                  }
               }
               else [[unlikely]] {
                  ctx.error = error_code::syntax_error;
                  return true;
               }

               break;
            }
            case '"': {
               return escaped;
            }
            default: {
               ++it;
            }
            }
         }
      }

      ctx.error = error_code::expected_quote;
      return false;
   }

   // very similar code to skip_till_quote, but it consumes the iterator and returns the key
   [[nodiscard]] GLZ_ALWAYS_INLINE const sv parse_unescaped_key(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

      auto start = it;

      for (const auto end_m7 = end - 7; it < end_m7; it += 8) {
         uint64_t chunk;
         std::memcpy(&chunk, it, 8);
         uint64_t test_chars = has_quote(chunk);
         if (test_chars) {
            it += (std::countr_zero(test_chars) >> 3);

            sv ret{start, size_t(it - start)};
            ++it;
            return ret;
         }
      }

      // Tail end of buffer. Should be rare we even get here
      while (it < end) {
         if (*it == '"') {
            sv ret{start, size_t(it - start)};
            ++it;
            return ret;
         }
         ++it;
      }
      ctx.error = error_code::expected_quote;
      return {};
   }

   struct key_stats_t
   {
      uint32_t min_length = (std::numeric_limits<uint32_t>::max)();
      uint32_t max_length{};
      uint32_t length_range{};
   };

   // consumes the iterator and returns the key
   // for error_on_unknown_keys = false, this may not return a valid key, in which case the iterator will not point to a
   // quote (")
   template <opts Opts, key_stats_t stats>
      requires(stats.length_range < 24)
   [[nodiscard]] GLZ_ALWAYS_INLINE const sv parse_key_cx(auto&& it) noexcept
   {
      static_assert(std::contiguous_iterator<std::decay_t<decltype(it)>>);

      static constexpr auto LengthRange = stats.length_range;

      auto start = it;

      if constexpr (Opts.error_on_unknown_keys) {
         it += stats.min_length; // immediately skip minimum length
      }
      else {
         // unknown keys must be searched for within the min_length

         // I don't want to support unknown keys having escaped quotes.
         // This would make everyone pay significant performance losses for an edge case that can be handled with known
         // keys.

         if constexpr (stats.min_length <= 8) {
            uint64_t chunk{};
            std::memcpy(&chunk, it, stats.min_length);
            const uint64_t test_chunk = has_quote(chunk);
            if (test_chunk) [[likely]] {
               it += (std::countr_zero(test_chunk) >> 3);
               return {start, size_t(it - start)};
            }
            it += stats.min_length;
         }
         else {
            auto e = it + stats.min_length;
            uint64_t chunk;
            for (const auto end_m7 = e - 7; it < end_m7; it += 8) {
               std::memcpy(&chunk, it, 8);
               const uint64_t test_chars = has_quote(chunk);
               if (test_chars) {
                  it += (std::countr_zero(test_chars) >> 3);
                  return {start, size_t(it - start)};
               }
            }

            while (it < e) {
               if (*it == '"') {
                  return {start, size_t(it - start)};
               }
               ++it;
            }
         }
      }

      if constexpr (LengthRange == 0) {
         return {start, stats.min_length};
      }
      else if constexpr (LengthRange == 1) {
         if (*it != '"') {
            ++it;
         }
         return {start, size_t(it - start)};
      }
      else if constexpr (LengthRange < 4) {
         for (const auto e = it + stats.length_range + 1; it < e; ++it) {
            if (*it == '"') {
               break;
            }
         }
         return {start, size_t(it - start)};
      }
      else if constexpr (LengthRange == 7) {
         uint64_t chunk; // no need to default initialize
         std::memcpy(&chunk, it, 8);
         const uint64_t test_chunk = has_quote(chunk);
         if (test_chunk) [[likely]] {
            it += (std::countr_zero(test_chunk) >> 3);
         }
         return {start, size_t(it - start)};
      }
      else if constexpr (LengthRange > 15) {
         uint64_t chunk; // no need to default initialize
         std::memcpy(&chunk, it, 8);
         uint64_t test_chunk = has_quote(chunk);
         if (test_chunk) {
            goto finish;
         }

         it += 8;
         std::memcpy(&chunk, it, 8);
         test_chunk = has_quote(chunk);
         if (test_chunk) {
            goto finish;
         }

         it += 8;
         static constexpr auto rest = LengthRange + 1 - 16;
         chunk = 0; // must zero out the chunk
         std::memcpy(&chunk, it, rest);
         test_chunk = has_quote(chunk);
         // If our chunk is zero, we have an invalid key (for error_on_unknown_keys = true)
         // We set the chunk to 1 so that we increment it by 0
         if (!test_chunk) {
            test_chunk = 1;
         }

      finish:
         it += (std::countr_zero(test_chunk) >> 3);
         return {start, size_t(it - start)};
      }
      else if constexpr (LengthRange > 7) {
         uint64_t chunk; // no need to default initialize
         std::memcpy(&chunk, it, 8);
         uint64_t test_chunk = has_quote(chunk);
         if (test_chunk) {
            it += (std::countr_zero(test_chunk) >> 3);
         }
         else {
            it += 8;
            static constexpr auto rest = LengthRange + 1 - 8;
            chunk = 0; // must zero out the chunk
            std::memcpy(&chunk, it, rest);
            test_chunk = has_quote(chunk);
            if (test_chunk) {
               it += (std::countr_zero(test_chunk) >> 3);
            }
         }
         return {start, size_t(it - start)};
      }
      else {
         uint64_t chunk{};
         std::memcpy(&chunk, it, LengthRange + 1);
         const uint64_t test_chunk = has_quote(chunk);
         if (test_chunk) [[likely]] {
            it += (std::countr_zero(test_chunk) >> 3);
         }
         return {start, size_t(it - start)};
      }
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE void skip_string(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      if constexpr (!Opts.opening_handled) {
         ++it;
      }

      if constexpr (Opts.force_conformance) {
         while (true) {
            if (*it < 16) [[unlikely]] {
               ctx.error = error_code::syntax_error;
               return;
            }

            switch (*it) {
            case '"': {
               ++it;
               return;
            }
            case '\\': {
               ++it;
               if (char_unescape_table[*it]) {
                  ++it;
                  continue;
               }
               else if (*it == 'u') {
                  ++it;
                  if (skip_unicode_code_point(it, end)) [[likely]] {
                     continue;
                  }
                  else [[unlikely]] {
                     ctx.error = error_code::unicode_escape_conversion_failure;
                     return;
                  }
               }
               ctx.error = error_code::syntax_error;
               return;
            }
            }
            ++it;
         }
      }
      else {
         for (const auto fin = end - 7; it < fin;) {
            uint64_t chunk;
            std::memcpy(&chunk, it, 8);
            const uint64_t test_chars = has_quote(chunk);
            if (test_chars) {
               it += (std::countr_zero(test_chars) >> 3);

               auto* prev = it - 1;
               while (*prev == '\\') {
                  --prev;
               }
               if (size_t(it - prev) % 2) {
                  ++it; // skip the quote
                  return;
               }
               ++it; // skip the escaped quote
            }
            else {
               it += 8;
            }
         }

         // Tail end of buffer. Should be rare we even get here
         while (it < end) {
            switch (*it) {
            case '\\': {
               ++it;
               if (it == end) [[unlikely]] {
                  ctx.error = error_code::expected_quote;
                  return;
               }
               ++it;
               break;
            }
            case '"': {
               auto* prev = it - 1;
               while (*prev == '\\') {
                  --prev;
               }
               if (size_t(it - prev) % 2) {
                  ++it; // skip the quote
                  return;
               }
               ++it; // skip the escaped quote
               break;
            }
            default: {
               ++it;
            }
            }
         }
      }
   }

   template <char open, char close>
   GLZ_ALWAYS_INLINE void skip_until_closed(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      ++it;
      size_t depth = 1;

      for (const auto fin = end - 7; it < fin;) {
         uint64_t chunk;
         std::memcpy(&chunk, it, 8);
         const uint64_t test = has_quote(chunk) | has_char<'/'>(chunk) | has_char<open>(chunk) | has_char<close>(chunk);
         if (test) {
            it += (std::countr_zero(test) >> 3);

            switch (*it) {
            case '"': {
               skip_string<opts{}>(ctx, it, end);
               if (bool(ctx.error)) [[unlikely]] {
                  return;
               }
               break;
            }
            case '/': {
               skip_comment(ctx, it, end);
               if (bool(ctx.error)) [[unlikely]] {
                  return;
               }
               break;
            }
            case open: {
               ++it;
               ++depth;
               break;
            }
            case close: {
               ++it;
               --depth;
               if (depth == 0) {
                  return;
               }
               break;
            }
            default: {
               ctx.error = error_code::unexpected_end;
               return;
            }
            }
         }
         else {
            it += 8;
         }
      }

      // Tail end of buffer. Should be rare we even get here
      while (it < end) {
         switch (*it) {
         case '"': {
            skip_string<opts{}>(ctx, it, end);
            if (bool(ctx.error)) [[unlikely]] {
               return;
            }
            break;
         }
         case '/': {
            skip_comment(ctx, it, end);
            if (bool(ctx.error)) [[unlikely]] {
               return;
            }
            break;
         }
         case open: {
            ++it;
            ++depth;
            break;
         }
         case close: {
            ++it;
            --depth;
            if (depth == 0) {
               return;
            }
            break;
         }
         default: {
            ++it;
         }
         }
      }

      ctx.error = error_code::unexpected_end;
   }

   inline constexpr std::optional<uint64_t> stoui(const std::string_view s) noexcept
   {
      if (s.empty()) {
         return {};
      }

      uint64_t ret;
      auto* c = s.data();
      bool valid = detail::stoui64(ret, c);
      if (valid) {
         return ret;
      }
      return {};
   }

   GLZ_ALWAYS_INLINE void skip_number_with_validation(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      it += *it == '-';
      const auto sig_start_it = it;
      auto frac_start_it = end;
      if (*it == '0') {
         ++it;
         if (*it != '.') {
            return;
         }
         ++it;
         goto frac_start;
      }
      it = std::find_if_not(it, end, is_digit);
      if (it == sig_start_it) {
         ctx.error = error_code::syntax_error;
         return;
      }
      if ((*it | ('E' ^ 'e')) == 'e') {
         ++it;
         goto exp_start;
      }
      if (*it != '.') return;
      ++it;
   frac_start:
      frac_start_it = it;
      it = std::find_if_not(it, end, is_digit);
      if (it == frac_start_it) {
         ctx.error = error_code::syntax_error;
         return;
      }
      if ((*it | ('E' ^ 'e')) != 'e') return;
      ++it;
   exp_start:
      it += *it == '+' || *it == '-';
      const auto exp_start_it = it;
      it = std::find_if_not(it, end, is_digit);
      if (it == exp_start_it) {
         ctx.error = error_code::syntax_error;
         return;
      }
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE void skip_number(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      if constexpr (!Opts.force_conformance) {
         while (numeric_table[*it]) {
            ++it;
         }
      }
      else {
         skip_number_with_validation(ctx, it, end);
      }
   }

   template <opts Opts>
   GLZ_ALWAYS_INLINE void skip_value(is_context auto&& ctx, auto&& it, auto&& end) noexcept;

   // expects opening whitespace to be handled
   GLZ_ALWAYS_INLINE sv parse_key(is_context auto&& ctx, auto&& it, auto&& end) noexcept
   {
      // TODO this assumes no escapes.
      if (bool(ctx.error)) [[unlikely]]
         return {};

      match<'"'>(ctx, it);
      if (bool(ctx.error)) [[unlikely]]
         return {};
      auto start = it;
      skip_till_quote(ctx, it, end);
      if (bool(ctx.error)) [[unlikely]]
         return {};
      return sv{start, static_cast<size_t>(it++ - start)};
   }

   /* Copyright (c) 2022 Tero 'stedo' Liukko, MIT License */
   GLZ_ALWAYS_INLINE unsigned char hex2dec(char hex) { return ((hex & 0xf) + (hex >> 6) * 9); }

   GLZ_ALWAYS_INLINE char32_t hex4_to_char32(const char* hex)
   {
      uint32_t value = hex2dec(hex[3]);
      value |= hex2dec(hex[2]) << 4;
      value |= hex2dec(hex[1]) << 8;
      value |= hex2dec(hex[0]) << 12;
      return value;
   }

   // errors return the 'in' pointer for better error reporting
   template <size_t Bytes>
      requires(Bytes == 8)
   GLZ_ALWAYS_INLINE const char* parse_string(const auto* in, auto* out, context& ctx) noexcept
   {
      uint64_t swar;
      while (true) {
         std::memcpy(&swar, in, Bytes);
         std::memcpy(out, in, Bytes);
         auto next = has_quote(swar) | has_escape(swar);

         if (next) {
            next = std::countr_zero(next) >> 3;
            auto escape_char = in[next];
            if (escape_char == '"') {
               return out + next;
            }
            else if (escape_char == '\\') {
               escape_char = in[next + 1];
               if (escape_char == 'u') [[unlikely]] {
                  in += next + 2;
                  out += next;
                  if (!handle_unicode_code_point(in, out)) {
                     ctx.error = error_code::unicode_escape_conversion_failure;
                     return in;
                  }
                  continue;
               }
               escape_char = char_unescape_table[escape_char];
               if (escape_char == 0) [[unlikely]] {
                  ctx.error = error_code::invalid_escape;
                  return in;
               }
               out[next] = escape_char;
               out += next + 1;
               in += next + 2;
            }
         }
         else {
            out += 8;
            in += 8;
         }
      }
   }

   // errors return the 'in' pointer for better error reporting
   template <size_t Bytes>
      requires(Bytes == 1)
   GLZ_ALWAYS_INLINE const char* parse_string(const auto* in, auto* out, context& ctx) noexcept
   {
      while (true) {
         *out = *in;
         if (*in == '"' || *in == '\\') {
            auto escape_char = *in;
            if (escape_char == '"') {
               return out;
            }
            else if (escape_char == '\\') {
               escape_char = in[1];
               if (escape_char == 'u') {
                  in += 2;
                  if (!handle_unicode_code_point(in, out)) {
                     ctx.error = error_code::unicode_escape_conversion_failure;
                     return in;
                  }
                  continue;
               }
               escape_char = char_unescape_table[escape_char];
               if (escape_char == 0) {
                  ctx.error = error_code::invalid_escape;
                  return in;
               }
               out[0] = escape_char;
               out += 1;
               in += 2;
            }
         }
         else {
            ++out;
            ++in;
         }
      }
   }

   template <size_t multiple>
   GLZ_ALWAYS_INLINE constexpr auto round_up_to_multiple(const std::integral auto val) noexcept
   {
      return val + (multiple - (val % multiple)) % multiple;
   }
}
