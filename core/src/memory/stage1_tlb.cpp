#include "niemu/memory/stage1_tlb.hpp"

namespace niemu::memory {
namespace {
constexpr std::uint64_t kPageOffsetMask = 0xFFFU;
}

bool Stage1Tlb::synchronize_configuration(const cpu::CpuState& state) noexcept {
    const auto& regs = state.translation_registers();
    if (!configuration_valid_ ||
        ttbr0_snapshot_ != regs.ttbr0_el1 ||
        tcr_snapshot_ != regs.tcr_el1 ||
        sctlr_snapshot_ != regs.sctlr_el1) {
        entries_.fill({});
        replacement_index_ = 0U;
        ttbr0_snapshot_ = regs.ttbr0_el1;
        tcr_snapshot_ = regs.tcr_el1;
        sctlr_snapshot_ = regs.sctlr_el1;
        configuration_valid_ = true;
        return true;
    }
    return false;
}

VirtualAddressTranslation Stage1Tlb::translate(
    const cpu::CpuState& state, const MemoryBus& physical_bus,
    std::uint64_t virtual_address, MemoryAccess access) noexcept {
    (void)synchronize_configuration(state);
    ++misses_;
    const auto virtual_page = virtual_address >> 12U;

    for (auto& entry : entries_) {
        if (entry.valid && entry.virtual_page == virtual_page &&
            entry.access == access) {
            ++hits_;
            --misses_;
            return {MmuFault::None,
                    (entry.physical_page << 12U) | (virtual_address & kPageOffsetMask),
                    entry.translation_level};
        }
    }

    const auto translated = Stage1Mmu::translate(
        state, physical_bus, virtual_address, access);
    if (!translated.ok()) return translated;

    auto& entry = entries_[replacement_index_];
    entry.virtual_page = virtual_page;
    entry.physical_page = translated.physical_address >> 12U;
    entry.access = access;
    entry.translation_level = translated.translation_level;
    entry.valid = true;
    replacement_index_ = (replacement_index_ + 1U) % kCapacity;
    return translated;
}

void Stage1Tlb::invalidate_all() noexcept {
    entries_.fill({});
    replacement_index_ = 0U;
}

void Stage1Tlb::invalidate_va(std::uint64_t virtual_address) noexcept {
    const auto virtual_page = virtual_address >> 12U;
    for (auto& entry : entries_) {
        if (entry.valid && entry.virtual_page == virtual_page) {
            entry.valid = false;
        }
    }
}

std::size_t Stage1Tlb::entry_count() const noexcept {
    std::size_t count = 0U;
    for (const auto& entry : entries_) {
        if (entry.valid) ++count;
    }
    return count;
}

} // namespace niemu::memory
