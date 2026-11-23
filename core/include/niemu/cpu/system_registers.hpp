#pragma once

#include "niemu/cpu/cpu_state.hpp"

#include <cstdint>
#include <optional>

namespace niemu::cpu {

enum class SystemRegister : std::uint16_t {
    NZCV = 0xDA10,
    DAIF = 0xDA11,
    CurrentEL = 0xC212,
    SP_EL0 = 0xC208,
    SP_EL1 = 0xE208,
    SP_EL2 = 0xE210,
    SP_EL3 = 0xE218,
    ELR_EL1 = 0xE201,
    ESR_EL1 = 0xE202,
    FAR_EL1 = 0xE203,
    TTBR0_EL1 = 0xE180,
    TCR_EL1 = 0xE181,
    MAIR_EL1 = 0xE182,
    SCTLR_EL1 = 0xE183
};

class SystemRegisters final {
public:
    [[nodiscard]] static std::optional<std::uint64_t>
    read(const CpuState& state, SystemRegister reg) noexcept;

    [[nodiscard]] static bool
    write(CpuState& state, SystemRegister reg, std::uint64_t value) noexcept;
};

} // namespace niemu::cpu
