#include "lib_portable.h"
#include "lib_portable_math.h"

#include <cstdint>
#include <iostream>

constexpr std::uint32_t kSampleLimit = 100U;

namespace sample_app
{
enum SampleMode : std::uint8_t
{
  SAMPLE_MODE_IDLE,
  SAMPLE_MODE_RUNNING
};

struct SampleState
{
  std::uint32_t totalCount_;
};

template <typename ValueType>
class SampleController
{
public:
  explicit SampleController(ValueType initial_value)
      : memberValue_(initial_value)
  {
  }

  auto add_value(ValueType new_value) -> ValueType
  {
    bool is_ready = new_value > 0U;
    ValueType local_value = is_ready ? new_value : 0U;
    return memberValue_ + local_value + kSampleLimit;
  }

private:
  ValueType memberValue_;
};
}

std::uint32_t sample_app_global_count = 0U;

auto sample_app_calculate(std::uint32_t input_value) -> std::uint32_t
{
  std::uint32_t result_value = input_value;

  switch(input_value)
  {
  case 0U:
    result_value = 1U;
    [[fallthrough]];
  case 1U:
    result_value = lib_portable_mathAdd(result_value, 2U);
    break;
  default:
    break;
  }

  return result_value;
}

auto main() -> decltype(0)
{
  sample_app::SampleState sample_state{sample_app_global_count};
  sample_app::SampleController<std::uint32_t> controller(
      sample_state.totalCount_);
  sample_app_global_count = lib_portable_increment(
      controller.add_value(sample_app_calculate(0U)));
  std::cout << sample_app_global_count << "\n";
  return sample_app_global_count > 0U ? 0 : 1;
}
