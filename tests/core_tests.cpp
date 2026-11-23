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


    using niemu::cpu::AArch64Decoder;
    using niemu::cpu::InstructionKind;
    using niemu::cpu::MoveWideOperation;
    using niemu::cpu::AddSubOperation;
    using niemu::cpu::BranchRegisterOperation;
    using niemu::cpu::LoadStoreOperation;

    const auto movz = AArch64Decoder::decode(0xD2824680U);
    if (movz.kind != InstructionKind::MoveWide || movz.move_wide_op != MoveWideOperation::MOVZ ||
        !movz.is_64_bit || movz.rd != 0U || movz.immediate != 0x1234U) return fail("MOVZ decode");
    const auto movk = AArch64Decoder::decode(0xF2C00020U);
    if (movk.kind != InstructionKind::MoveWide || movk.move_wide_op != MoveWideOperation::MOVK ||
        movk.shift != 32U) return fail("MOVK decode");
    const auto add = AArch64Decoder::decode(0x91001420U);
    if (add.kind != InstructionKind::AddSubImmediate || add.add_sub_op != AddSubOperation::ADD ||
        add.rn != 1U || add.rd != 0U || add.immediate != 5U) return fail("ADD immediate decode");
    const auto subs = AArch64Decoder::decode(0xF100041FU);
    if (subs.kind != InstructionKind::AddSubImmediate || subs.add_sub_op != AddSubOperation::SUBS ||
        !subs.set_flags || subs.rd != 31U || subs.immediate != 1U) return fail("SUBS immediate decode");
    const auto branch = AArch64Decoder::decode(0x14000004U);
    if (branch.kind != InstructionKind::BranchImmediate || branch.link ||
        branch.signed_immediate != 16) return fail("B immediate decode");
    const auto bl = AArch64Decoder::decode(0x94000002U);
    if (bl.kind != InstructionKind::BranchImmediate || !bl.link ||
        bl.signed_immediate != 8) return fail("BL immediate decode");
    const auto bcond = AArch64Decoder::decode(0x54000040U);
    if (bcond.kind != InstructionKind::BranchConditional || bcond.condition != 0U ||
        bcond.signed_immediate != 8) return fail("B.cond decode");
    const auto br = AArch64Decoder::decode(0xD61F0060U);
    if (br.kind != InstructionKind::BranchRegister || br.branch_reg_op != BranchRegisterOperation::BR ||
        br.rn != 3U) return fail("BR decode");
    const auto ret = AArch64Decoder::decode(0xD65F03C0U);
    if (ret.kind != InstructionKind::BranchRegister || ret.branch_reg_op != BranchRegisterOperation::RET ||
        ret.rn != 30U) return fail("RET decode");
    const auto ldr = AArch64Decoder::decode(0xF9400820U);
    if (ldr.kind != InstructionKind::LoadStoreUnsignedImmediate ||
        ldr.load_store_op != LoadStoreOperation::LDR || ldr.access_size != 8U ||
        ldr.immediate != 16U) return fail("LDR unsigned immediate decode");
    const auto str = AArch64Decoder::decode(0xB9000C62U);
    if (str.kind != InstructionKind::LoadStoreUnsignedImmediate ||
        str.load_store_op != LoadStoreOperation::STR || str.access_size != 4U ||
        str.immediate != 12U) return fail("STR unsigned immediate decode");
    if (AArch64Decoder::decode(0xFFFFFFFFU).kind != InstructionKind::Unsupported)
        return fail("unsupported instruction classification");

    std::cout << "Niemu CPU and decoder tests passed\n";
    return EXIT_SUCCESS;
}
