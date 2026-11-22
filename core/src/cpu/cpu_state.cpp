#include "niemu/cpu/cpu_state.hpp"

namespace niemu::cpu {

void CpuState::reset() noexcept {
    x_.fill(0);
    sp_el_.fill(0);
    pc_ = 0;
    current_el_ = ExceptionLevel::EL1;
    pstate_ = {};
    pstate_.d = true;
    pstate_.a = true;
    pstate_.i = true;
    pstate_.f = true;
}

std::uint64_t CpuState::read_x(std::size_t index) const noexcept {
    return index < x_.size() ? x_[index] : 0;
}

void CpuState::write_x(std::size_t index, std::uint64_t value) noexcept {
    if (index < x_.size()) {
        x_[index] = value;
    }
}

std::uint64_t CpuState::read_sp(ExceptionLevel level) const noexcept {
    return sp_el_[el_index(level) < sp_el_.size() ? el_index(level) : 0U];
}

void CpuState::write_sp(ExceptionLevel level, std::uint64_t value) noexcept {
    const auto index = el_index(level);
    if (index < sp_el_.size()) {
        sp_el_[index] = value;
    }
}

std::uint64_t CpuState::read_sp_current() const noexcept {
    return read_sp(current_el_);
}

void CpuState::write_sp_current(std::uint64_t value) noexcept {
    write_sp(current_el_, value);
}

std::uint64_t CpuState::read_reg(std::uint8_t index, bool reg31_is_sp) const noexcept {
    if (index < kGeneralRegisterCount) {
        return x_[index];
    }
    return reg31_is_sp ? read_sp_current() : 0;
}

void CpuState::write_reg(std::uint8_t index, bool reg31_is_sp, std::uint64_t value) noexcept {
    if (index < kGeneralRegisterCount) {
        x_[index] = value;
    } else if (reg31_is_sp) {
        write_sp_current(value);
    }
}

} // namespace niemu::cpu
