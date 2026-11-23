#pragma once

#include "niemu/cpu/cpu_state.hpp"
#include "niemu/memory/address_space.hpp"

#include <cstdint>

namespace niemu::memory {

enum class MmuFault : std::uint8_t {
    None,
    Disabled,
    UnsupportedConfiguration,
    AddressSizeFault,
    TranslationFault,
    AccessFlagFault,
    PermissionFault,
    PhysicalAccessFault
};

struct VirtualAddressTranslation {
    MmuFault fault{MmuFault::None};
    std::uint64_t physical_address{0};
    std::uint8_t translation_level{0};
    [[nodiscard]] constexpr bool ok() const noexcept { return fault == MmuFault::None; }
};

class Stage1Mmu final {
public:
    // Implements the EL1&0 TTBR0 regime subset: 4KB granule, 39-bit VA,
    // three translation levels (L1-L3), EL1 read/write/execute permissions.
    [[nodiscard]] static VirtualAddressTranslation translate(
        const cpu::CpuState& state, const MemoryBus& physical_bus,
        std::uint64_t virtual_address, MemoryAccess access) noexcept;
};

} // namespace niemu::memory
