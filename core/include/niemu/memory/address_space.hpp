#pragma once

#include "niemu/memory/memory_bus.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace niemu::memory {

enum class MemoryAccess : std::uint8_t { Read, Write, Execute };

enum class MemoryPermission : std::uint8_t {
    None = 0,
    Read = 1U << 0U,
    Write = 1U << 1U,
    Execute = 1U << 2U
};

[[nodiscard]] constexpr MemoryPermission operator|(
    MemoryPermission lhs, MemoryPermission rhs) noexcept {
    return static_cast<MemoryPermission>(
        static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

[[nodiscard]] constexpr bool has_permission(
    MemoryPermission permissions, MemoryPermission requested) noexcept {
    return (static_cast<std::uint8_t>(permissions) &
            static_cast<std::uint8_t>(requested)) ==
           static_cast<std::uint8_t>(requested);
}

struct AddressTranslation {
    MemoryFault fault{MemoryFault::None};
    GuestMemory* memory{nullptr};
    std::uint64_t backing_address{0};
    [[nodiscard]] constexpr bool ok() const noexcept {
        return fault == MemoryFault::None && memory != nullptr;
    }
};

class GuestAddressSpace final {
public:
    [[nodiscard]] MemoryAccessResult map_ram(
        std::uint64_t guest_base, std::uint64_t size, GuestMemory& backing,
        std::uint64_t backing_offset = 0U,
        MemoryPermission permissions = MemoryPermission::Read |
                                      MemoryPermission::Write |
                                      MemoryPermission::Execute);

    [[nodiscard]] AddressTranslation translate(
        std::uint64_t guest_address, std::size_t length,
        MemoryAccess access) const noexcept;

    [[nodiscard]] std::size_t region_count() const noexcept { return regions_.size(); }

private:
    struct Region {
        std::uint64_t base;
        std::uint64_t size;
        GuestMemory* backing;
        std::uint64_t backing_offset;
        MemoryPermission permissions;
    };

    std::vector<Region> regions_;
};

} // namespace niemu::memory
