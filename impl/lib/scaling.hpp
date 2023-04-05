#pragma once

#include <cstdint>
#include <type_traits>

namespace scaling {
  template <typename T, int O> struct int_scaling;

  template <typename T, int O>
  using int_scaling_t = typename int_scaling<T, O>::type;

  template <typename T>
  requires std::is_integral_v<T>
  struct int_scaling<T, 0>  { using type = T; };

  template <> struct int_scaling<int8_t, 1> { using type = int16_t; };
  template <> struct int_scaling<int16_t, 1> { using type = int32_t; };
  template <> struct int_scaling<int32_t, 1> { using type = int64_t; };
  template <> struct int_scaling<int64_t, 1> { using type = int64_t; };

  template <> struct int_scaling<uint8_t, 1> { using type = uint16_t; };
  template <> struct int_scaling<uint16_t, 1> { using type = uint32_t; };
  template <> struct int_scaling<uint32_t, 1> { using type = uint64_t; };
  template <> struct int_scaling<uint64_t, 1> { using type = uint64_t; };

  template <> struct int_scaling<int8_t, -1> { using type = int8_t; };
  template <> struct int_scaling<int16_t, -1> { using type = int8_t; };
  template <> struct int_scaling<int32_t, -1> { using type = int16_t; };
  template <> struct int_scaling<int64_t, -1> { using type = int32_t; };

  template <> struct int_scaling<uint8_t, -1> { using type = uint8_t; };
  template <> struct int_scaling<uint16_t, -1> { using type = uint8_t; };
  template <> struct int_scaling<uint32_t, -1> { using type = uint16_t; };
  template <> struct int_scaling<uint64_t, -1> { using type = uint32_t; };

  template <typename T, int O>
  requires std::is_integral_v<T> && (O > 1)
  struct int_scaling<T, O> {
    using type = int_scaling_t<int_scaling_t<T, O - 1>, 1>;
  };

  template <typename T, int O>
  requires std::is_integral_v<T> && (O < -1)
  struct int_scaling<T, O> {
    using type = int_scaling_t<int_scaling_t<T, O + 1>, -1>;
  };
}
