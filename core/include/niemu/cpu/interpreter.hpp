#pragma once

#include "niemu/cpu/cpu_state.hpp"
#include "niemu/cpu/decoder.hpp"
#include "niemu/memory/memory_bus.hpp"
#include "niemu/memory/stage1_tlb.hpp"

#include <cstdint>

namespace niemu::cpu {

enum class ExecutionStatus : std::uint8_t {
    Executed,
    UnsupportedInstruction,
    RequiresMemoryBus,
    MemoryAccessFault,
    MemoryAddressOverflow,
    RequiresTlb,
    PrivilegeFault
};

class AArch64Interpreter final {
public:
    [[nodiscard]] static ExecutionStatus execute(
        CpuState& state, const DecodedInstruction& instruction,
        memory::MemoryBus* memory_bus = nullptr,
        memory::Stage1Tlb* stage1_tlb = nullptr) noexcept;
};

} // namespace niemu::cpu
