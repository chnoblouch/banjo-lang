#ifndef BANJO_TARGET_X86_64_ENCODER_H
#define BANJO_TARGET_X86_64_ENCODER_H

#include "banjo/emit/binary_builder.hpp"
#include "banjo/emit/binary_module.hpp"

#include <cstdint>
#include <ostream>
#include <variant>
#include <vector>

namespace banjo {

namespace target {

class X8664Encoder final : public BinaryBuilder {

private:
    enum RegCode : std::uint8_t {
        EAX = 0,
        ECX = 1,
        EDX = 2,
        EBX = 3,
        ESP = 4,
        EBP = 5,
        ESI = 6,
        EDI = 7,
        R8 = 8,
        R9 = 9,
        R10 = 10,
        R11 = 11,
        R12 = 12,
        R13 = 13,
        R14 = 14,
        R15 = 15,
        XMM0 = 0,
        XMM1 = 1,
        XMM2 = 2,
        XMM3 = 3,
        XMM4 = 4,
        XMM5 = 5,
        XMM6 = 6,
        XMM7 = 7,
        XMM8 = 8,
        XMM9 = 9,
        XMM10 = 10,
        XMM11 = 11,
        XMM12 = 12,
        XMM13 = 13,
        XMM14 = 14,
        XMM15 = 15,
    };

    enum AddrRegCode : std::uint8_t {
        ADDR_NO_BASE = 255,
        ADDR_EAX = 0,
        ADDR_ECX = 1,
        ADDR_EDX = 2,
        ADDR_EBX = 3,
        ADDR_ESP = 4,
        ADDR_EBP = 5,
        ADDR_ESI = 6,
        ADDR_EDI = 7,
        ADDR_R8 = 8,
        ADDR_R9 = 9,
        ADDR_R10 = 10,
        ADDR_R11 = 11,
        ADDR_R12 = 12,
        ADDR_R13 = 13,
        ADDR_R14 = 14,
        ADDR_R15 = 15,
        ADDR_NO_INDEX = 0b100
    };

    struct Immediate {
        std::uint64_t value;
        std::int64_t symbol_index;
    };

    struct RegAddress {
        std::uint8_t scale;
        AddrRegCode index;
        AddrRegCode base;
        std::int32_t displacement;
    };

    struct SymbolAddress {
        std::uint32_t symbol_index;
        BinSymbolUseKind use_kind;
        std::int32_t displacement;
    };

    typedef std::variant<RegAddress, SymbolAddress> Address;
    typedef std::variant<RegCode, Address> RegOrAddr;

    struct BasicInstrOpcodes {
        std::uint8_t digit;
        std::uint8_t rax_imm8;
        std::uint8_t rax_imm16;
        std::uint8_t rm8_imm8;
        std::uint8_t rm16_imm16;
        std::uint8_t rm16_imm8;
        std::uint8_t rm8_r8;
        std::uint8_t rm16_r16;
        std::uint8_t r8_rm8;
        std::uint8_t r16_rm16;
    };

private:
    void encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo &frame_info) override;

    void encode_mov(mcode::Instruction &instr, mcode::Function *func);
    void encode_movsx(mcode::Instruction &instr, mcode::Function *func);
    void encode_movzx(mcode::Instruction &instr, mcode::Function *func);
    void encode_add(mcode::Instruction &instr, mcode::Function *func);
    void encode_sub(mcode::Instruction &instr, mcode::Function *func);
    void encode_imul(mcode::Instruction &instr, mcode::Function *func);
    void encode_idiv(mcode::Instruction &instr, mcode::Function *func);
    void encode_and(mcode::Instruction &instr, mcode::Function *func);
    void encode_or(mcode::Instruction &instr, mcode::Function *func);
    void encode_xor(mcode::Instruction &instr, mcode::Function *func);
    void encode_shl(mcode::Instruction &instr, mcode::Function *func);
    void encode_shr(mcode::Instruction &instr, mcode::Function *func);
    void encode_cdq();
    void encode_cqo();
    void encode_jmp(mcode::Instruction &instr);
    void encode_cmp(mcode::Instruction &instr, mcode::Function *func);
    void encode_je(mcode::Instruction &instr);
    void encode_jne(mcode::Instruction &instr);
    void encode_ja(mcode::Instruction &instr);
    void encode_jae(mcode::Instruction &instr);
    void encode_jb(mcode::Instruction &instr);
    void encode_jbe(mcode::Instruction &instr);
    void encode_jg(mcode::Instruction &instr);
    void encode_jge(mcode::Instruction &instr);
    void encode_jl(mcode::Instruction &instr);
    void encode_jle(mcode::Instruction &instr);
    void encode_cmove(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovne(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmova(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovae(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovb(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovbe(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovg(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovge(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovl(mcode::Instruction &instr, mcode::Function *func);
    void encode_cmovle(mcode::Instruction &instr, mcode::Function *func);
    void encode_lea(mcode::Instruction &instr, mcode::Function *func);
    void encode_call(mcode::Instruction &instr, mcode::Function *func);
    void encode_ret();
    void encode_push(mcode::Instruction &instr);
    void encode_pop(mcode::Instruction &instr);
    void encode_movss(mcode::Instruction &instr, mcode::Function *func);
    void encode_movsd(mcode::Instruction &instr, mcode::Function *func);
    void encode_movaps(mcode::Instruction &instr, mcode::Function *func);
    void encode_movups(mcode::Instruction &instr, mcode::Function *func);
    void encode_movq(mcode::Instruction &instr);
    void encode_addss(mcode::Instruction &instr, mcode::Function *func);
    void encode_addsd(mcode::Instruction &instr, mcode::Function *func);
    void encode_subss(mcode::Instruction &instr, mcode::Function *func);
    void encode_subsd(mcode::Instruction &instr, mcode::Function *func);
    void encode_mulss(mcode::Instruction &instr, mcode::Function *func);
    void encode_mulsd(mcode::Instruction &instr, mcode::Function *func);
    void encode_divss(mcode::Instruction &instr, mcode::Function *func);
    void encode_divsd(mcode::Instruction &instr, mcode::Function *func);
    void encode_xorps(mcode::Instruction &instr, mcode::Function *func);
    void encode_xorpd(mcode::Instruction &instr, mcode::Function *func);
    void encode_minss(mcode::Instruction &instr, mcode::Function *func);
    void encode_maxss(mcode::Instruction &instr, mcode::Function *func);
    void encode_sqrtss(mcode::Instruction &instr, mcode::Function *func);
    void encode_ucomiss(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtss2sd(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtsd2ss(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtsi2ss(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtsi2sd(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtss2si(mcode::Instruction &instr, mcode::Function *func);
    void encode_cvtsd2si(mcode::Instruction &instr, mcode::Function *func);

    void encode_basic_instr(mcode::Instruction &instr, mcode::Function *func, const BasicInstrOpcodes &opcodes);

    void encode_shift(mcode::Instruction &instr, mcode::Function *func, std::uint8_t digit);
    void encode_jcc(mcode::Instruction &instr, std::uint8_t opcode);
    void encode_cmovcc(mcode::Instruction &instr, mcode::Function *func, std::uint8_t opcode);

    void encode_sse_op(mcode::Instruction &instr, mcode::Function *func, std::uint8_t prefix, std::uint8_t opcode);

    void encode_sse_cvt(
        mcode::Instruction &instr,
        mcode::Function *func,
        std::uint8_t prefix,
        std::uint8_t opcode,
        std::uint8_t size
    );

    void emit_mov_rr(RegCode dst, RegCode src, std::uint8_t size);
    void emit_mov_ri(RegCode dst, Immediate imm, std::uint8_t size);
    void emit_mov_rm(RegCode dst, Address src, std::uint8_t size);
    void emit_mov_mr(Address dst, RegCode src, std::uint8_t size);
    void emit_mov_mi(Address dst, Immediate imm, std::uint8_t size);
    void emit_imul_rr(RegCode dst, RegCode src, std::uint8_t size);
    void emit_imul_rm(RegCode dst, Address src, std::uint8_t size);
    void emit_imul_rri(RegCode dst, Immediate imm, std::uint8_t size);
    void emit_lea_rm(RegCode dst, Address src, std::uint8_t size);
    void emit_ret();

    void emit_basic_rr(std::uint8_t opcode8, std::uint8_t opcode32, RegCode dst, RegCode src, std::uint8_t size);

    void emit_basic_ri(
        std::uint8_t opcode8,
        std::uint8_t opcode32,
        std::uint8_t opcode_imm8,
        std::uint8_t modrm_reg_digit,
        RegCode dst,
        Immediate imm,
        std::uint8_t size
    );

    void emit_basic_rm(std::uint8_t opcode8, std::uint8_t opcode32, RegCode dst, Address src, std::uint8_t size);
    void emit_basic_mr(std::uint8_t opcode8, std::uint8_t opcode32, Address dst, RegCode src, std::uint8_t size);

    void emit_basic_mi(
        std::uint8_t opcode8,
        std::uint8_t opcode32,
        std::uint8_t opcode_imm8,
        std::uint8_t modrm_reg_digit,
        Address dst,
        Immediate imm,
        std::uint8_t size
    );

    void emit_cmovcc(std::uint8_t opcode, RegCode dst, RegOrAddr src, std::uint8_t size);
    void emit_sse(std::uint8_t prefix, std::uint8_t opcode, RegCode dst, RegOrAddr src, std::uint8_t size);

    void emit_opcode(std::uint8_t opcode);
    void emit_mem_reg(Address addr, RegCode reg);
    void emit_mem_digit(Address addr, std::uint8_t digit, std::uint32_t offset_to_next_instr = 0);
    void emit_mem_digit(RegAddress addr, std::uint8_t digit);
    void emit_mem_digit(SymbolAddress addr, std::uint8_t digit, std::uint32_t offset_to_next_instr);
    void emit_combined_opcode(std::uint8_t opcode, std::uint8_t reg);
    void emit_modrm_rr(std::uint8_t reg, RegCode rm);
    void emit_modrm_sib(std::uint8_t reg, RegOrAddr roa);
    void emit_16bit_prefix_if_required(std::uint8_t size);
    void emit_rex_r(std::uint8_t size, RegCode rm);
    void emit_rex_rr(std::uint8_t size, std::uint8_t reg, RegCode rm);
    void emit_rex_rm(std::uint8_t size, std::uint8_t reg, Address addr);
    void emit_rex_rroa(std::uint8_t size, std::uint8_t reg, RegOrAddr roa);
    void emit_rex_o(std::uint8_t size, std::uint8_t opcode_ext);

    void emit_modrm(std::uint8_t mod, std::uint8_t reg, std::uint8_t rm);
    void emit_sib(std::uint8_t scale, std::uint8_t index, std::uint8_t base);
    void emit_16bit_prefix();
    void emit_rex(bool w, bool r, bool x, bool b);

    RegCode reg(mcode::Operand &operand);
    Immediate imm(mcode::Operand &operand);
    Address addr(mcode::Operand &operand, mcode::Function *func);
    RegOrAddr roa(mcode::Operand &operand, mcode::Function *func);
    bool is_reg(mcode::Operand &operand);
    bool is_imm(mcode::Operand &operand);
    bool is_addr(mcode::Operand &operand);
    bool is_roa(mcode::Operand &operand);
    RegCode reg(mcode::Register reg);
    BinSymbolUseKind use_kind(const mcode::Symbol &symbol);

    void process_eh_pushreg(mcode::Instruction &instr, UnwindInfo &frame_info);

    void apply_relaxation() override;
    void resolve_internal_symbols() override;
    void resolve_relaxable_slice(SectionBuilder::SectionSlice &slice);
    void resolve_symbol(SectionBuilder::SectionSlice &slice, SymbolUse &use);
    void relax_jmp(std::uint32_t slice_index);
    void relax_jcc(SymbolUse &use, std::uint32_t slice_index);
    std::int32_t compute_branch_displacement(SectionBuilder::SectionSlice &branch_slice);

    bool fits_in_i8(std::int64_t value);
    bool fits_in_32_bits(Immediate imm);
};

} // namespace target

} // namespace banjo

#endif
