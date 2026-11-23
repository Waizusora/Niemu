#pragma once

#include <cstdint>

namespace niemu::cpu {

enum class InstructionKind : std::uint8_t {
    Unsupported,
    MoveWide,
    AddSubImmediate,
    BranchImmediate,
    BranchConditional,
    BranchRegister,
    LoadStoreUnsignedImmediate,
    Tlbi
};

enum class MoveWideOperation : std::uint8_t { MOVN, MOVZ, MOVK };
enum class AddSubOperation : std::uint8_t { ADD, ADDS, SUB, SUBS };
enum class BranchRegisterOperation : std::uint8_t { BR, BLR, RET };
enum class LoadStoreOperation : std::uint8_t { STR, LDR };
enum class TlbiOperation : std::uint8_t { VMALLE1, VAE1 };

struct DecodedInstruction {
    InstructionKind kind{InstructionKind::Unsupported};
    std::uint32_t raw{0};
    std::uint8_t rd{0};
    std::uint8_t rn{0};
    std::uint8_t rm{0};
    std::uint8_t condition{0};
    std::uint8_t shift{0};
    std::uint8_t access_size{0};
    bool is_64_bit{false};
    bool link{false};
    bool set_flags{false};
    std::uint64_t immediate{0};
    std::int64_t signed_immediate{0};
    MoveWideOperation move_wide_op{MoveWideOperation::MOVZ};
    AddSubOperation add_sub_op{AddSubOperation::ADD};
    BranchRegisterOperation branch_reg_op{BranchRegisterOperation::BR};
    LoadStoreOperation load_store_op{LoadStoreOperation::STR};
    TlbiOperation tlbi_op{TlbiOperation::VMALLE1};
};

class AArch64Decoder final {
public:
    [[nodiscard]] static DecodedInstruction decode(std::uint32_t instruction) noexcept;
};

} // namespace niemu::cpu
