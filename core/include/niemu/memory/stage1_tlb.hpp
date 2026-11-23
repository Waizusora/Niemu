#pragma once

#include "niemu/memory/stage1_mmu.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace niemu::memory {

class Stage1Tlb final {
public:
    static constexpr std::size_t kCapacity = 64U;

    [[nodiscard]] VirtualAddressTranslation translate(
        const cpu::CpuState& state, const MemoryBus& physical_bus,
        std::uint64_t virtual_address, MemoryAccess access) noexcept;

    // Invalidates all cached translations, for example after a guest TLBI ALL.
    void invalidate_all() noexcept;

    // Invalidates the cached 4KB virtual page for every access type.
    void invalidate_va(std::uint64_t virtual_address) noexcept;

    [[nodiscard]] std::size_t entry_count() const noexcept;
    [[nodiscard]] std::uint64_t hit_count() const noexcept { return hits_; }
    [[nodiscard]] std::uint64_t miss_count() const noexcept { return misses_; }

private:
    struct Entry {
        std::uint64_t virtual_page{0};
        std::uint64_t physical_page{0};
        MemoryAccess access{MemoryAccess::Read};
        std::uint8_t translation_level{0};
        bool valid{false};
    };

    [[nodiscard]] bool synchronize_configuration(
        const cpu::CpuState& state) noexcept;

    std::array<Entry, kCapacity> entries_{};
    std::size_t replacement_index_{0};
    std::uint64_t ttbr0_snapshot_{0};
    std::uint64_t tcr_snapshot_{0};
    std::uint64_t sctlr_snapshot_{0};
    std::uint64_t hits_{0};
    std::uint64_t misses_{0};
    bool configuration_valid_{false};
};

} // namespace niemu::memory
