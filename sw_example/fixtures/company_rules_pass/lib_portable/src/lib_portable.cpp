#include "lib_portable.h"

#include "lib_portable_math.h"

std::uint32_t lib_portable_counter = 0U;

auto lib_portable_increment(std::uint32_t value) -> std::uint32_t
{
  lib_portable_counter = lib_portable_mathAdd(value, 1U);
  return lib_portable_counter;
}
