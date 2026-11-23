#include "niemu/cpu/system_registers.hpp"

namespace niemu::cpu {

std::optional<std::uint64_t>
SystemRegisters::read(const CpuState& state, SystemRegister reg) noexcept {
    switch (reg) {
    case SystemRegister::NZCV:
        return static_cast<std::uint64_t>(state.pstate().nzcv()) << 28U;
    case SystemRegister::DAIF:
        return (static_cast<std::uint64_t>(state.pstate().d) << 9U) |
               (static_cast<std::uint64_t>(state.pstate().a) << 8U) |
               (static_cast<std::uint64_t>(state.pstate().i) << 7U) |
               (static_cast<std::uint64_t>(state.pstate().f) << 6U);
    case SystemRegister::CurrentEL:
        return static_cast<std::uint64_t>(state.current_el()) << 2U;
    case SystemRegister::SP_EL0: return state.read_sp(ExceptionLevel::EL0);
    case SystemRegister::SP_EL1: return state.read_sp(ExceptionLevel::EL1);
    case SystemRegister::SP_EL2: return state.read_sp(ExceptionLevel::EL2);
    case SystemRegister::SP_EL3: return state.read_sp(ExceptionLevel::EL3);
    case SystemRegister::TTBR0_EL1: return state.translation_registers().ttbr0_el1;
    case SystemRegister::TCR_EL1: return state.translation_registers().tcr_el1;
    case SystemRegister::MAIR_EL1: return state.translation_registers().mair_el1;
    case SystemRegister::SCTLR_EL1: return state.translation_registers().sctlr_el1;
    default:
        return std::nullopt;
    }
}

bool SystemRegisters::write(CpuState& state, SystemRegister reg, std::uint64_t value) noexcept {
    switch (reg) {
    case SystemRegister::NZCV:
        state.pstate().set_nzcv(static_cast<std::uint8_t>((value >> 28U) & 0xFU));
        return true;
    case SystemRegister::DAIF:
        state.pstate().d = ((value >> 9U) & 1U) != 0U;
        state.pstate().a = ((value >> 8U) & 1U) != 0U;
        state.pstate().i = ((value >> 7U) & 1U) != 0U;
        state.pstate().f = ((value >> 6U) & 1U) != 0U;
        return true;
    case SystemRegister::SP_EL0: state.write_sp(ExceptionLevel::EL0, value); return true;
    case SystemRegister::SP_EL1: state.write_sp(ExceptionLevel::EL1, value); return true;
    case SystemRegister::SP_EL2: state.write_sp(ExceptionLevel::EL2, value); return true;
    case SystemRegister::SP_EL3: state.write_sp(ExceptionLevel::EL3, value); return true;
    case SystemRegister::TTBR0_EL1: state.translation_registers().ttbr0_el1 = value; return true;
    case SystemRegister::TCR_EL1: state.translation_registers().tcr_el1 = value; return true;
    case SystemRegister::MAIR_EL1: state.translation_registers().mair_el1 = value; return true;
    case SystemRegister::SCTLR_EL1: state.translation_registers().sctlr_el1 = value; return true;
    default:
        return false;
    }
}

} // namespace niemu::cpu
