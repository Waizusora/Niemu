#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace niemu::cpu {

enum class ExceptionLevel : std::uint8_t {
    EL0 = 0, EL1 = 1, EL2 = 2, EL3 = 3
};

struct PState {
    bool n{false};
    bool z{false};
    bool c{false};
    bool v{false};
    bool d{false};
    bool a{false};
    bool i{false};
    bool f{false};
    bool single_step{false};
    bool illegal_execution_state{false};

    [[nodiscard]] constexpr std::uint8_t nzcv() const noexcept {
        return static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(n) << 3U) |
            (static_cast<std::uint8_t>(z) << 2U) |
            (static_cast<std::uint8_t>(c) << 1U) |
            static_cast<std::uint8_t>(v));
    }

    constexpr void set_nzcv(std::uint8_t value) noexcept {
        n = (value & 0x8U) != 0U;
        z = (value & 0x4U) != 0U;
        c = (value & 0x2U) != 0U;
        v = (value & 0x1U) != 0U;
    }

    [[nodiscard]] constexpr std::uint64_t pack() const noexcept;
};

class CpuState final {
public:
    static constexpr std::size_t kGeneralRegisterCount = 31;

    CpuState() noexcept { reset(); }

    void reset() noexcept;

    [[nodiscard]] std::uint64_t read_x(std::size_t index) const noexcept;
    void write_x(std::size_t index, std::uint64_t value) noexcept;

    [[nodiscard]] std::uint64_t read_sp(ExceptionLevel level) const noexcept;
    void write_sp(ExceptionLevel level, std::uint64_t value) noexcept;

    [[nodiscard]] std::uint64_t read_sp_current() const noexcept;
    void write_sp_current(std::uint64_t value) noexcept;

    [[nodiscard]] std::uint64_t pc() const noexcept { return pc_; }
    void set_pc(std::uint64_t value) noexcept { pc_ = value; }

    [[nodiscard]] ExceptionLevel current_el() const noexcept { return current_el_; }
    void set_current_el(ExceptionLevel level) noexcept { current_el_ = level; }

    [[nodiscard]] PState& pstate() noexcept { return pstate_; }
    [[nodiscard]] const PState& pstate() const noexcept { return pstate_; }

    // AArch64 register encoding 31 means SP or XZR/WZR depending on the instruction.
    [[nodiscard]] std::uint64_t read_reg(std::uint8_t index, bool reg31_is_sp) const noexcept;
    void write_reg(std::uint8_t index, bool reg31_is_sp, std::uint64_t value) noexcept;

private:
    [[nodiscard]] static constexpr std::size_t el_index(ExceptionLevel level) noexcept {
        return static_cast<std::size_t>(level);
    }

    std::array<std::uint64_t, kGeneralRegisterCount> x_{};
    std::array<std::uint64_t, 4> sp_el_{};
    std::uint64_t pc_{0};
    ExceptionLevel current_el_{ExceptionLevel::EL1};
    PState pstate_{};
};

constexpr std::uint64_t PState::pack() const noexcept {
    return (static_cast<std::uint64_t>(nzcv()) << 28U) |
           (static_cast<std::uint64_t>(d) << 9U) |
           (static_cast<std::uint64_t>(a) << 8U) |
           (static_cast<std::uint64_t>(i) << 7U) |
           (static_cast<std::uint64_t>(f) << 6U) |
           (static_cast<std::uint64_t>(single_step) << 21U) |
           (static_cast<std::uint64_t>(illegal_execution_state) << 20U);
}

} // namespace niemu::cpu
