#include "niemu/cpu/interpreter.hpp"

#include <limits>

namespace niemu::cpu {
namespace {

[[nodiscard]] constexpr std::uint64_t width_mask(bool is_64_bit) noexcept {
    return is_64_bit ? ~std::uint64_t{0} : 0xFFFFFFFFULL;
}

[[nodiscard]] constexpr bool condition_passed(const PState& flags,
                                               std::uint8_t condition) noexcept {
    if (condition >= 0xEU) return true;
    const bool result = [&]() noexcept {
        switch (condition & 0xEU) {
        case 0x0U: return flags.z;
        case 0x2U: return flags.c;
        case 0x4U: return flags.n;
        case 0x6U: return flags.v;
        case 0x8U: return flags.c && !flags.z;
        case 0xAU: return flags.n == flags.v;
        case 0xCU: return !flags.z && (flags.n == flags.v);
        default: return true;
        }
    }();
    return (condition & 1U) != 0U ? !result : result;
}

void set_add_flags(PState& flags, std::uint64_t lhs, std::uint64_t rhs,
                   std::uint64_t result, bool is_64_bit) noexcept {
    const auto sign = is_64_bit ? (std::uint64_t{1} << 63U)
                                : (std::uint64_t{1} << 31U);
    flags.n = (result & sign) != 0U;
    flags.z = result == 0U;
    flags.c = is_64_bit ? result < lhs
                        : ((lhs + rhs) >> 32U) != 0U;
    flags.v = ((~(lhs ^ rhs) & (lhs ^ result) & sign) != 0U);
}

void set_sub_flags(PState& flags, std::uint64_t lhs, std::uint64_t rhs,
                   std::uint64_t result, bool is_64_bit) noexcept {
    const auto sign = is_64_bit ? (std::uint64_t{1} << 63U)
                                : (std::uint64_t{1} << 31U);
    flags.n = (result & sign) != 0U;
    flags.z = result == 0U;
    flags.c = lhs >= rhs;
    flags.v = (((lhs ^ rhs) & (lhs ^ result) & sign) != 0U);
}

} // namespace

ExecutionStatus AArch64Interpreter::execute(
    CpuState& state, const DecodedInstruction& instruction,
    memory::MemoryBus* memory_bus, memory::Stage1Tlb* stage1_tlb) noexcept {
    const auto pc = state.pc();
    const auto next_pc = pc + 4U;

    switch (instruction.kind) {
    case InstructionKind::MoveWide: {
        const auto mask = width_mask(instruction.is_64_bit);
        const auto shifted = (instruction.immediate & 0xFFFFU) << instruction.shift;
        std::uint64_t value = 0;
        if (instruction.move_wide_op == MoveWideOperation::MOVN) {
            value = ~shifted;
        } else if (instruction.move_wide_op == MoveWideOperation::MOVZ) {
            value = shifted;
        } else {
            const auto old = state.read_reg(instruction.rd, false);
            const auto lane_mask = 0xFFFFULL << instruction.shift;
            value = (old & ~lane_mask) | shifted;
        }
        state.write_reg(instruction.rd, false, value & mask);
        state.set_pc(next_pc);
        return ExecutionStatus::Executed;
    }

    case InstructionKind::AddSubImmediate: {
        const auto mask = width_mask(instruction.is_64_bit);
        const auto lhs = state.read_reg(instruction.rn, true) & mask;
        const auto rhs = instruction.immediate & mask;
        const bool subtract = instruction.add_sub_op == AddSubOperation::SUB ||
                              instruction.add_sub_op == AddSubOperation::SUBS;
        const auto result = (subtract ? lhs - rhs : lhs + rhs) & mask;
        if (instruction.set_flags) {
            if (subtract) set_sub_flags(state.pstate(), lhs, rhs, result,
                                         instruction.is_64_bit);
            else set_add_flags(state.pstate(), lhs, rhs, result,
                               instruction.is_64_bit);
        }
        state.write_reg(instruction.rd, !instruction.set_flags, result);
        state.set_pc(next_pc);
        return ExecutionStatus::Executed;
    }

    case InstructionKind::BranchImmediate:
        if (instruction.link) state.write_x(30U, next_pc);
        state.set_pc(pc + static_cast<std::uint64_t>(instruction.signed_immediate));
        return ExecutionStatus::Executed;

    case InstructionKind::BranchConditional:
        state.set_pc(condition_passed(state.pstate(), instruction.condition)
                         ? pc + static_cast<std::uint64_t>(instruction.signed_immediate)
                         : next_pc);
        return ExecutionStatus::Executed;

    case InstructionKind::BranchRegister: {
        const auto target = state.read_x(instruction.rn);
        if (instruction.link) state.write_x(30U, next_pc);
        state.set_pc(target);
        return ExecutionStatus::Executed;
    }

    case InstructionKind::LoadStoreUnsignedImmediate: {
        if (memory_bus == nullptr) return ExecutionStatus::RequiresMemoryBus;
        const auto base_address = state.read_reg(instruction.rn, true);
        if (base_address > std::numeric_limits<std::uint64_t>::max() -
                               instruction.immediate) {
            return ExecutionStatus::MemoryAddressOverflow;
        }
        const auto address = base_address + instruction.immediate;
        if (instruction.load_store_op == LoadStoreOperation::LDR) {
            std::uint64_t value = 0U;
            const auto result = memory_bus->read(address, instruction.access_size, value);
            if (!result.ok()) return ExecutionStatus::MemoryAccessFault;
            state.write_reg(instruction.rd, false, value);
        } else {
            const auto value = state.read_reg(instruction.rd, false);
            const auto result = memory_bus->write(address, instruction.access_size, value);
            if (!result.ok()) return ExecutionStatus::MemoryAccessFault;
        }
        state.set_pc(next_pc);
        return ExecutionStatus::Executed;
    }

    case InstructionKind::Tlbi:
        if (state.current_el() != ExceptionLevel::EL1)
            return ExecutionStatus::PrivilegeFault;
        if (stage1_tlb == nullptr) return ExecutionStatus::RequiresTlb;
        if (instruction.tlbi_op == TlbiOperation::VMALLE1) {
            stage1_tlb->invalidate_all();
        } else if (instruction.tlbi_op == TlbiOperation::VAE1) {
            constexpr std::uint64_t kVaOperandMask = 0x00FFFFFFFFFFF000ULL;
            const auto operand = state.read_x(instruction.rn);
            const auto asid = static_cast<std::uint16_t>((operand >> 48U) & 0xFFFFU);
            stage1_tlb->invalidate_va_asid(operand & kVaOperandMask, asid);
        } else if (instruction.tlbi_op == TlbiOperation::ASIDE1) {
            const auto asid = static_cast<std::uint16_t>(
                (state.read_x(instruction.rn) >> 48U) & 0xFFFFU);
            stage1_tlb->invalidate_asid(asid);
        } else {
            return ExecutionStatus::UnsupportedInstruction;
        }
        state.set_pc(next_pc);
        return ExecutionStatus::Executed;

    case InstructionKind::Unsupported:
    default:
        return ExecutionStatus::UnsupportedInstruction;
    }
}

} // namespace niemu::cpu
