#include "niemu/memory/stage1_mmu.hpp"

namespace niemu::memory {
namespace {
constexpr std::uint64_t kVaLimit = 1ULL << 39U;
constexpr std::uint64_t kAddressMask = 0x0000FFFFFFFFF000ULL;
constexpr std::uint64_t kDescriptorValid = 1ULL;
constexpr std::uint64_t kDescriptorTypeMask = 3ULL;
constexpr std::uint64_t kAccessFlag = 1ULL << 10U;
constexpr std::uint64_t kApEl1ReadOnly = 1ULL << 7U;
constexpr std::uint64_t kPxn = 1ULL << 53U;
constexpr std::uint64_t kUxn = 1ULL << 54U;

[[nodiscard]] constexpr VirtualAddressTranslation fault(
    MmuFault reason, std::uint8_t level = 0U) noexcept {
    return {reason, 0U, level};
}
}

VirtualAddressTranslation Stage1Mmu::translate(
    const cpu::CpuState& state, const MemoryBus& physical_bus,
    std::uint64_t virtual_address, MemoryAccess access) noexcept {
    const auto& regs = state.translation_registers();
    if ((regs.sctlr_el1 & 1U) == 0U) return fault(MmuFault::Disabled);

    // T0SZ=25 selects a 39-bit input range. TG0=00 selects 4KB pages.
    // EPD0 must be clear. Other TCR controls and MAIR attributes are not
    // consumed by this deliberately bounded first implementation.
    const auto t0sz = regs.tcr_el1 & 0x3FU;
    const auto tg0 = (regs.tcr_el1 >> 14U) & 3U;
    const bool epd0 = ((regs.tcr_el1 >> 7U) & 1U) != 0U;
    if (t0sz != 25U || tg0 != 0U || epd0) {
        return fault(MmuFault::UnsupportedConfiguration);
    }
    if (virtual_address >= kVaLimit) return fault(MmuFault::AddressSizeFault);

    std::uint64_t table = regs.ttbr0_el1 & kAddressMask;
    constexpr unsigned shifts[] = {30U, 21U, 12U};
    for (std::uint8_t level = 1U; level <= 3U; ++level) {
        const auto shift = shifts[level - 1U];
        const auto index = (virtual_address >> shift) & 0x1FFU;
        const auto descriptor_address = table + index * 8U;
        std::uint64_t descriptor = 0U;
        const auto read_result = physical_bus.read(descriptor_address, 8U, descriptor);
        if (!read_result.ok()) return fault(MmuFault::PhysicalAccessFault, level);

        if ((descriptor & kDescriptorValid) == 0U) {
            return fault(MmuFault::TranslationFault, level);
        }
        const auto type = descriptor & kDescriptorTypeMask;
        const bool is_table = type == 3U && level < 3U;
        const bool is_block = type == 1U && level < 3U;
        const bool is_page = type == 3U && level == 3U;
        if (!is_table && !is_block && !is_page) {
            return fault(MmuFault::TranslationFault, level);
        }
        if ((descriptor & kAccessFlag) == 0U) {
            return fault(MmuFault::AccessFlagFault, level);
        }

        if (is_table) {
            table = descriptor & kAddressMask;
            continue;
        }

        const bool read_only = (descriptor & kApEl1ReadOnly) != 0U;
        if (access == MemoryAccess::Write && read_only) {
            return fault(MmuFault::PermissionFault, level);
        }
        if (access == MemoryAccess::Execute &&
            ((descriptor & kPxn) != 0U)) {
            return fault(MmuFault::PermissionFault, level);
        }

        const auto page_size_mask = (1ULL << shift) - 1ULL;
        const auto output_base = descriptor & kAddressMask & ~page_size_mask;
        const auto physical = output_base | (virtual_address & page_size_mask);
        return {MmuFault::None, physical, level};
    }
    return fault(MmuFault::TranslationFault, 3U);
}

} // namespace niemu::memory
