#include "niemu/cpu/decoder.hpp"

namespace niemu::cpu {
namespace {

[[nodiscard]] constexpr std::int64_t sign_extend(std::uint64_t value,
                                                  unsigned bits) noexcept {
    const std::uint64_t sign = std::uint64_t{1} << (bits - 1U);
    const std::uint64_t mask = (std::uint64_t{1} << bits) - 1U;
    value &= mask;
    return static_cast<std::int64_t>((value ^ sign) - sign);
}

[[nodiscard]] constexpr DecodedInstruction base(std::uint32_t raw) noexcept {
    DecodedInstruction result{};
    result.raw = raw;
    return result;
}

} // namespace

DecodedInstruction AArch64Decoder::decode(std::uint32_t instruction) noexcept {
    auto out = base(instruction);

    // Move wide immediate: MOVN, MOVZ, MOVK.
    if ((instruction & 0x1F800000U) == 0x12800000U) {
        const auto opc = static_cast<std::uint8_t>((instruction >> 29U) & 0x3U);
        if (opc == 0x3U) return out;
        out.kind = InstructionKind::MoveWide;
        out.is_64_bit = (instruction >> 31U) != 0U;
        out.move_wide_op = opc == 0U ? MoveWideOperation::MOVN
                         : opc == 2U ? MoveWideOperation::MOVZ
                                     : MoveWideOperation::MOVK;
        out.shift = static_cast<std::uint8_t>(((instruction >> 21U) & 0x3U) * 16U);
        if (!out.is_64_bit && out.shift >= 32U) return base(instruction);
        out.immediate = (instruction >> 5U) & 0xFFFFU;
        out.rd = static_cast<std::uint8_t>(instruction & 0x1FU);
        return out;
    }

    // Add/subtract immediate, including flag-setting variants.
    if ((instruction & 0x1F000000U) == 0x11000000U) {
        out.kind = InstructionKind::AddSubImmediate;
        out.is_64_bit = (instruction >> 31U) != 0U;
        const bool subtract = ((instruction >> 30U) & 1U) != 0U;
        out.set_flags = ((instruction >> 29U) & 1U) != 0U;
        out.add_sub_op = subtract
            ? (out.set_flags ? AddSubOperation::SUBS : AddSubOperation::SUB)
            : (out.set_flags ? AddSubOperation::ADDS : AddSubOperation::ADD);
        out.shift = ((instruction >> 22U) & 1U) != 0U ? 12U : 0U;
        out.immediate = static_cast<std::uint64_t>((instruction >> 10U) & 0xFFFU)
                        << out.shift;
        out.rn = static_cast<std::uint8_t>((instruction >> 5U) & 0x1FU);
        out.rd = static_cast<std::uint8_t>(instruction & 0x1FU);
        return out;
    }

    // Unconditional branch immediate: B and BL.
    if ((instruction & 0x7C000000U) == 0x14000000U) {
        out.kind = InstructionKind::BranchImmediate;
        out.link = ((instruction >> 31U) & 1U) != 0U;
        out.signed_immediate = sign_extend(instruction & 0x03FFFFFFU, 26U) * 4;
        return out;
    }

    // Conditional branch immediate.
    if ((instruction & 0xFF000010U) == 0x54000000U) {
        out.kind = InstructionKind::BranchConditional;
        out.condition = static_cast<std::uint8_t>(instruction & 0xFU);
        out.signed_immediate =
            sign_extend((instruction >> 5U) & 0x7FFFFU, 19U) * 4;
        return out;
    }

    // Register branches: BR, BLR, RET.
    const auto branch_encoding = instruction & 0xFFFFFC1FU;
    if (branch_encoding == 0xD61F0000U ||
        branch_encoding == 0xD63F0000U ||
        branch_encoding == 0xD65F0000U) {
        out.kind = InstructionKind::BranchRegister;
        out.rn = static_cast<std::uint8_t>((instruction >> 5U) & 0x1FU);
        out.branch_reg_op = branch_encoding == 0xD61F0000U
            ? BranchRegisterOperation::BR
            : branch_encoding == 0xD63F0000U
                ? BranchRegisterOperation::BLR
                : BranchRegisterOperation::RET;
        out.link = out.branch_reg_op == BranchRegisterOperation::BLR;
        return out;
    }

    // Load/store unsigned immediate, integer GPR class (STR/LDR only).
    if ((instruction & 0x3B000000U) == 0x39000000U) {
        const auto opc = static_cast<std::uint8_t>((instruction >> 22U) & 0x3U);
        if (opc > 1U) return out;
        out.kind = InstructionKind::LoadStoreUnsignedImmediate;
        out.access_size = static_cast<std::uint8_t>(1U << ((instruction >> 30U) & 0x3U));
        out.load_store_op = opc == 0U ? LoadStoreOperation::STR
                                      : LoadStoreOperation::LDR;
        out.immediate = static_cast<std::uint64_t>((instruction >> 10U) & 0xFFFU)
                        * out.access_size;
        out.rn = static_cast<std::uint8_t>((instruction >> 5U) & 0x1FU);
        out.rd = static_cast<std::uint8_t>(instruction & 0x1FU);
        out.is_64_bit = out.access_size == 8U;
        return out;
    }

    return out;
}

} // namespace niemu::cpu
