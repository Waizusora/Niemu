#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace niemu::memory {

enum class MemoryFault : std::uint8_t {
    None,
    OutOfBounds,
    AddressOverflow,
    UnalignedAccess,
    InvalidWidth,
    NullBuffer
};

enum class UnalignedAccessPolicy : std::uint8_t {
    Reject,
    Allow
};

struct MemoryAccessResult {
    MemoryFault fault{MemoryFault::None};
    [[nodiscard]] constexpr bool ok() const noexcept {
        return fault == MemoryFault::None;
    }
};

class GuestMemory final {
public:
    explicit GuestMemory(std::size_t size,
                         UnalignedAccessPolicy policy = UnalignedAccessPolicy::Reject);

    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }
    [[nodiscard]] UnalignedAccessPolicy unaligned_policy() const noexcept { return policy_; }

    [[nodiscard]] MemoryAccessResult read_bytes(
        std::uint64_t address, std::uint8_t* destination, std::size_t length) const noexcept;
    [[nodiscard]] MemoryAccessResult write_bytes(
        std::uint64_t address, const std::uint8_t* source, std::size_t length) noexcept;

private:
    [[nodiscard]] MemoryAccessResult validate(
        std::uint64_t address, std::size_t length) const noexcept;

    std::vector<std::uint8_t> bytes_;
    UnalignedAccessPolicy policy_;
};

class MemoryBus final {
public:
    explicit MemoryBus(GuestMemory& memory) noexcept : memory_(memory) {}

    [[nodiscard]] MemoryAccessResult read(
        std::uint64_t address, std::uint8_t width, std::uint64_t& value) const noexcept;
    [[nodiscard]] MemoryAccessResult write(
        std::uint64_t address, std::uint8_t width, std::uint64_t value) noexcept;

private:
    GuestMemory& memory_;
};

} // namespace niemu::memory
