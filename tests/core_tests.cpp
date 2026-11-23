#include "niemu/cpu/cpu_state.hpp"
#include "niemu/cpu/decoder.hpp"
#include "niemu/cpu/system_registers.hpp"
#include "niemu/version.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
int fail(const char* message) {
    std::cerr << "FAIL: " << message << '\n';
    return EXIT_FAILURE;
}
}

int main() {
    using niemu::cpu::CpuState;
    using niemu::cpu::ExceptionLevel;
    using niemu::cpu::SystemRegister;
    using niemu::cpu::SystemRegisters;

    if (niemu::version() != std::string_view{"0.1.0"}) {
        return fail("version mismatch");
    }

    CpuState state;
    if (state.current_el() != ExceptionLevel::EL1 || state.pc() != 0U) {
        return fail("reset architectural state");
    }
    if (!state.pstate().d || !state.pstate().a ||
        !state.pstate().i || !state.pstate().f) {
        return fail("reset DAIF masks");
    }

    state.write_x(0U, 0x123456789ABCDEF0ULL);
    if (state.read_x(0U) != 0x123456789ABCDEF0ULL) {
        return fail("X register read/write");
    }
    state.write_x(31U, 7U);
    if (state.read_x(31U) != 0U) {
        return fail("X register bounds / XZR invariant");
    }

    state.write_sp_current(0x1000U);
    if (state.read_reg(31U, true) != 0x1000U ||
        state.read_reg(31U, false) != 0U) {
        return fail("register 31 SP/XZR selection");
    }
    state.write_reg(31U, false, 0x55U);
    if (state.read_sp_current() != 0x1000U) {
        return fail("XZR write must be discarded");
    }
    if (state.read_reg(32U, true) != 0U) {
        return fail("invalid register index must read as zero");
    }
    state.write_reg(32U, true, 0x99U);
    if (state.read_sp_current() != 0x1000U) {
        return fail("invalid register index must not alias SP");
    }

    state.pstate().set_nzcv(0xAU);
    const auto nzcv = SystemRegisters::read(state, SystemRegister::NZCV);
    if (!nzcv || *nzcv != (0xAULL << 28U)) {
        return fail("NZCV system register read");
    }
    if (!SystemRegisters::write(state, SystemRegister::NZCV, 0x5ULL << 28U) ||
        state.pstate().nzcv() != 0x5U) {
        return fail("NZCV system register write");
    }

    if (!SystemRegisters::write(state, SystemRegister::SP_EL2, 0x2000U)) {
        return fail("SP_EL2 write");
    }
    const auto sp_el2 = SystemRegisters::read(state, SystemRegister::SP_EL2);
    if (!sp_el2 || *sp_el2 != 0x2000U) {
        return fail("SP_EL2 read");
    }

    if (SystemRegisters::read(state, SystemRegister::ELR_EL1).has_value()) {
        return fail("unimplemented system register must be unavailable");
    }
    if (SystemRegisters::write(state, SystemRegister::ELR_EL1, 0U)) {
        return fail("unimplemented system register write must fail");
    }

    std::cout << "Niemu CPU foundation tests passed\n";
    return EXIT_SUCCESS;
}
