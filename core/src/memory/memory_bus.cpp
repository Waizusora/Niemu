#include "niemu/memory/memory_bus.hpp"
#include "niemu/memory/address_space.hpp"

#include <limits>

namespace niemu::memory {

GuestMemory::GuestMemory(std::size_t size, UnalignedAccessPolicy policy)
    : bytes_(size, 0U), policy_(policy) {}

MemoryAccessResult GuestMemory::validate(
    std::uint64_t address, std::size_t length) const noexcept {
    if (length == 0U) return {};
    if (address > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return {MemoryFault::OutOfBounds};
    }
    const auto offset = static_cast<std::size_t>(address);
    if (offset > bytes_.size() || length > bytes_.size() - offset) {
        return {MemoryFault::OutOfBounds};
    }
    return {};
}

MemoryAccessResult GuestMemory::read_bytes(
    std::uint64_t address, std::uint8_t* destination, std::size_t length) const noexcept {
    if (length != 0U && destination == nullptr) return {MemoryFault::NullBuffer};
    const auto result = validate(address, length);
    if (!result.ok()) return result;
    const auto offset = static_cast<std::size_t>(address);
    for (std::size_t i = 0; i < length; ++i) destination[i] = bytes_[offset + i];
    return {};
}

MemoryAccessResult GuestMemory::write_bytes(
    std::uint64_t address, const std::uint8_t* source, std::size_t length) noexcept {
    if (length != 0U && source == nullptr) return {MemoryFault::NullBuffer};
    const auto result = validate(address, length);
    if (!result.ok()) return result;
    const auto offset = static_cast<std::size_t>(address);
    for (std::size_t i = 0; i < length; ++i) bytes_[offset + i] = source[i];
    return {};
}

MemoryAccessResult MemoryBus::read(
    std::uint64_t address, std::uint8_t width, std::uint64_t& value) const noexcept {
    if (width != 1U && width != 2U && width != 4U && width != 8U) {
        return {MemoryFault::InvalidWidth};
    }
    GuestMemory* target = memory_;
    std::uint64_t target_address = address;
    if (address_space_ != nullptr) {
        const auto translated = address_space_->translate(address, width, MemoryAccess::Read);
        if (!translated.ok()) return {translated.fault};
        target = translated.memory;
        target_address = translated.backing_address;
    }
    if (target == nullptr) return {MemoryFault::UnmappedAddress};
    if (target->unaligned_policy() == UnalignedAccessPolicy::Reject && width > 1U &&
        (address % width) != 0U) return {MemoryFault::UnalignedAccess};
    std::uint8_t bytes[8]{};
    const auto result = target->read_bytes(target_address, bytes, width);
    if (!result.ok()) return result;
    std::uint64_t loaded = 0U;
    for (std::uint8_t i = 0U; i < width; ++i) {
        loaded |= static_cast<std::uint64_t>(bytes[i]) << (static_cast<unsigned>(i) * 8U);
    }
    value = loaded;
    return {};
}

MemoryAccessResult MemoryBus::write(
    std::uint64_t address, std::uint8_t width, std::uint64_t value) noexcept {
    if (width != 1U && width != 2U && width != 4U && width != 8U) {
        return {MemoryFault::InvalidWidth};
    }
    GuestMemory* target = memory_;
    std::uint64_t target_address = address;
    if (address_space_ != nullptr) {
        const auto translated = address_space_->translate(address, width, MemoryAccess::Write);
        if (!translated.ok()) return {translated.fault};
        target = translated.memory;
        target_address = translated.backing_address;
    }
    if (target == nullptr) return {MemoryFault::UnmappedAddress};
    if (target->unaligned_policy() == UnalignedAccessPolicy::Reject && width > 1U &&
        (address % width) != 0U) return {MemoryFault::UnalignedAccess};
    std::uint8_t bytes[8]{};
    for (std::uint8_t i = 0U; i < width; ++i) {
        bytes[i] = static_cast<std::uint8_t>(
            (value >> (static_cast<unsigned>(i) * 8U)) & 0xFFU);
    }
    return target->write_bytes(target_address, bytes, width);
}

} // namespace niemu::memory
