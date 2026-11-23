#pragma once

#include "niemu/cpu/cpu_state.hpp"
#include "niemu/cpu/decoder.hpp"

#include <cstdint>

namespace niemu::cpu {

enum class ExecutionStatus : std::uint8_t {
    Executed,
    UnsupportedInstruction,
    RequiresMemoryBus
};

class AArch64Interpreter final {
public:
    [[nodiscard]] static ExecutionStatus execute(
        CpuState& state, const DecodedInstruction& instruction) noexcept;
};

} // namespace niemu::cpu
