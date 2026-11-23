#include "niemu/cpu/cpu_state.hpp"
#include "niemu/cpu/decoder.hpp"
#include "niemu/cpu/interpreter.hpp"
#include "niemu/cpu/system_registers.hpp"
#include "niemu/memory/memory_bus.hpp"
#include "niemu/memory/address_space.hpp"
#include "niemu/version.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
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


    using niemu::cpu::AArch64Interpreter;
    using niemu::cpu::ExecutionStatus;

    CpuState exec_state;
    exec_state.set_pc(0x1000U);
    const auto movz_exec = AArch64Decoder::decode(0xD2824680U);
    if (AArch64Interpreter::execute(exec_state, movz_exec) != ExecutionStatus::Executed ||
        exec_state.read_x(0U) != 0x1234U || exec_state.pc() != 0x1004U)
        return fail("interpreter MOVZ execution");

    exec_state.write_x(1U, 7U);
    const auto add_exec = AArch64Decoder::decode(0x91001420U);
    if (AArch64Interpreter::execute(exec_state, add_exec) != ExecutionStatus::Executed ||
        exec_state.read_x(0U) != 12U || exec_state.pc() != 0x1008U)
        return fail("interpreter ADD immediate execution");

    exec_state.set_pc(0x2000U);
    const auto bl_exec = AArch64Decoder::decode(0x94000002U);
    if (AArch64Interpreter::execute(exec_state, bl_exec) != ExecutionStatus::Executed ||
        exec_state.pc() != 0x2008U || exec_state.read_x(30U) != 0x2004U)
        return fail("interpreter BL execution");

    exec_state.set_pc(0x3000U);
    exec_state.pstate().z = true;
    const auto beq_exec = AArch64Decoder::decode(0x54000040U);
    if (AArch64Interpreter::execute(exec_state, beq_exec) != ExecutionStatus::Executed ||
        exec_state.pc() != 0x3008U)
        return fail("interpreter taken B.EQ");

    exec_state.set_pc(0x4000U);
    exec_state.pstate().z = false;
    if (AArch64Interpreter::execute(exec_state, beq_exec) != ExecutionStatus::Executed ||
        exec_state.pc() != 0x4004U)
        return fail("interpreter not-taken B.EQ");

    exec_state.set_pc(0x5000U);
    const auto ldr_exec = AArch64Decoder::decode(0xF9400820U);
    if (AArch64Interpreter::execute(exec_state, ldr_exec) != ExecutionStatus::RequiresMemoryBus ||
        exec_state.pc() != 0x5000U)
        return fail("interpreter defers memory instruction");

    using niemu::memory::GuestMemory;
    using niemu::memory::MemoryBus;
    using niemu::memory::MemoryFault;
    using niemu::memory::UnalignedAccessPolicy;

    GuestMemory guest_memory(64U);
    MemoryBus bus(guest_memory);
    if (!bus.write(16U, 8U, 0x1122334455667788ULL).ok())
        return fail("guest memory 64-bit write");
    std::uint64_t memory_value = 0U;
    if (!bus.read(16U, 8U, memory_value).ok() ||
        memory_value != 0x1122334455667788ULL)
        return fail("guest memory little-endian scalar read");
    if (guest_memory.read_bytes(16U, nullptr, 1U).fault != MemoryFault::NullBuffer)
        return fail("guest memory null buffer validation");
    if (bus.read(64U, 8U, memory_value).fault != MemoryFault::OutOfBounds)
        return fail("guest memory upper-bound validation");
    if (bus.read(std::numeric_limits<std::uint64_t>::max() - 7U, 8U, memory_value).fault !=
        MemoryFault::OutOfBounds)
        return fail("guest memory large-address validation");
    if (bus.read(0U, 3U, memory_value).fault != MemoryFault::InvalidWidth)
        return fail("guest memory width validation");
    if (bus.read(3U, 4U, memory_value).fault != MemoryFault::UnalignedAccess)
        return fail("guest memory alignment policy");

    GuestMemory unaligned_memory(32U, UnalignedAccessPolicy::Allow);
    MemoryBus unaligned_bus(unaligned_memory);
    if (!unaligned_bus.write(3U, 4U, 0xA1B2C3D4U).ok() ||
        !unaligned_bus.read(3U, 4U, memory_value).ok() ||
        memory_value != 0xA1B2C3D4U)
        return fail("guest memory configurable unaligned access");

    CpuState memory_state;
    memory_state.set_pc(0x6000U);
    memory_state.write_x(1U, 0U);
    memory_state.write_x(0U, 0xFFFFFFFFFFFFFFFFULL);
    if (AArch64Interpreter::execute(memory_state, AArch64Decoder::decode(0xB9401020U),
                                   &bus) != ExecutionStatus::Executed ||
        memory_state.read_x(0U) != 0x55667788U || memory_state.pc() != 0x6004U)
        return fail("interpreter W-register LDR zero extension");

    memory_state.set_pc(0x6100U);
    memory_state.write_x(3U, 60U);
    memory_state.write_x(2U, 0xAABBCCDDU);
    if (AArch64Interpreter::execute(memory_state, str, &bus) !=
            ExecutionStatus::MemoryAccessFault ||
        memory_state.pc() != 0x6100U)
        return fail("interpreter STR fault preserves PC");

    memory_state.set_pc(0x6200U);
    memory_state.write_x(3U, 16U);
    if (AArch64Interpreter::execute(memory_state, str, &bus) !=
            ExecutionStatus::Executed ||
        memory_state.pc() != 0x6204U ||
        !bus.read(28U, 4U, memory_value).ok() || memory_value != 0xAABBCCDDU)
        return fail("interpreter STR W register");

    memory_state.set_pc(0x6300U);
    memory_state.write_x(1U, 56U);
    if (AArch64Interpreter::execute(memory_state, ldr_exec, &bus) !=
            ExecutionStatus::MemoryAccessFault ||
        memory_state.pc() != 0x6300U)
        return fail("interpreter LDR out-of-bounds fault preserves PC");

    memory_state.set_pc(0x6400U);
    memory_state.write_x(1U, std::numeric_limits<std::uint64_t>::max() - 7U);
    if (AArch64Interpreter::execute(memory_state, ldr_exec, &bus) !=
            ExecutionStatus::MemoryAddressOverflow ||
        memory_state.pc() != 0x6400U)
        return fail("interpreter effective-address overflow");

    using niemu::memory::GuestAddressSpace;
    using niemu::memory::MemoryAccess;
    using niemu::memory::MemoryPermission;

    GuestMemory mapped_backing(128U);
    GuestAddressSpace address_space;
    const auto rw = MemoryPermission::Read | MemoryPermission::Write;
    const auto rx = MemoryPermission::Read | MemoryPermission::Execute;
    if (!address_space.map_ram(0x1000U, 64U, mapped_backing, 16U, rw).ok())
        return fail("map RAM region");
    if (!address_space.map_ram(0x2000U, 32U, mapped_backing, 80U, rx).ok())
        return fail("map second RAM region");
    if (address_space.region_count() != 2U)
        return fail("address space region count");
    if (address_space.map_ram(0x1030U, 16U, mapped_backing).fault !=
        MemoryFault::AddressOverflow)
        return fail("reject overlapping RAM regions");
    if (address_space.map_ram(std::numeric_limits<std::uint64_t>::max(), 2U,
                              mapped_backing).fault != MemoryFault::OutOfBounds)
        return fail("reject guest region address overflow");
    if (address_space.translate(0x3000U, 4U, MemoryAccess::Read).fault !=
        MemoryFault::UnmappedAddress)
        return fail("unmapped guest physical address");
    if (address_space.translate(0x2000U, 4U, MemoryAccess::Write).fault !=
        MemoryFault::PermissionDenied)
        return fail("read-only region write permission");
    if (address_space.translate(0x2000U, 4U, MemoryAccess::Execute).fault !=
        MemoryFault::None)
        return fail("executable region permission");

    MemoryBus mapped_bus(address_space);
    if (!mapped_bus.write(0x1008U, 4U, 0xCAFEBABEU).ok() ||
        !mapped_bus.read(0x1008U, 4U, memory_value).ok() ||
        memory_value != 0xCAFEBABEU)
        return fail("address-space bus routing");
    if (mapped_bus.write(0x2000U, 4U, 0x1234U).fault != MemoryFault::PermissionDenied)
        return fail("address-space bus enforces write permission");
    if (mapped_bus.read(0x103EU, 4U, memory_value).fault != MemoryFault::UnmappedAddress)
        return fail("access cannot cross mapped region boundary");

    std::cout << "Niemu CPU and decoder tests passed\n";
    return EXIT_SUCCESS;
}
