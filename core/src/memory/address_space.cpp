#include "niemu/memory/address_space.hpp"

#include <limits>

namespace niemu::memory {
namespace {
[[nodiscard]] constexpr MemoryPermission permission_for(MemoryAccess access) noexcept {
    switch (access) {
    case MemoryAccess::Read: return MemoryPermission::Read;
    case MemoryAccess::Write: return MemoryPermission::Write;
    case MemoryAccess::Execute: return MemoryPermission::Execute;
    }
    return MemoryPermission::None;
}
}

MemoryAccessResult GuestAddressSpace::map_ram(
    std::uint64_t guest_base, std::uint64_t size, GuestMemory& backing,
    std::uint64_t backing_offset, MemoryPermission permissions) noexcept {
    if (size == 0U || guest_base > std::numeric_limits<std::uint64_t>::max() - (size - 1U) ||
        backing_offset > static_cast<std::uint64_t>(backing.size()) ||
        size > static_cast<std::uint64_t>(backing.size()) - backing_offset) {
        return {MemoryFault::OutOfBounds};
    }
    const auto new_end = guest_base + size;
    for (const auto& region : regions_) {
        const auto region_end = region.base + region.size;
        if (guest_base < region_end && region.base < new_end) {
            return {MemoryFault::AddressOverflow};
        }
    }
    regions_.push_back({guest_base, size, &backing, backing_offset, permissions});
    return {};
}

AddressTranslation GuestAddressSpace::translate(
    std::uint64_t guest_address, std::size_t length, MemoryAccess access) const noexcept {
    if (length == 0U) return {MemoryFault::None, nullptr, 0U};
    const auto count = static_cast<std::uint64_t>(length);
    if (guest_address > std::numeric_limits<std::uint64_t>::max() - (count - 1U)) {
        return {MemoryFault::AddressOverflow, nullptr, 0U};
    }
    for (const auto& region : regions_) {
        if (guest_address < region.base) continue;
        const auto offset = guest_address - region.base;
        if (offset >= region.size || count > region.size - offset) continue;
        if (!has_permission(region.permissions, permission_for(access))) {
            return {MemoryFault::PermissionDenied, nullptr, 0U};
        }
        return {MemoryFault::None, region.backing, region.backing_offset + offset};
    }
    return {MemoryFault::UnmappedAddress, nullptr, 0U};
}

} // namespace niemu::memory
