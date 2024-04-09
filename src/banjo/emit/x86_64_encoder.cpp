#include "x86_64_encoder.hpp"

#include "target/x86_64/x86_64_opcode.hpp"
#include "target/x86_64/x86_64_register.hpp"

#include <iostream>
#include <limits>

constexpr unsigned MAX_RELAXATION_PASSES = 4;

BinModule X8664Encoder::encode(mcode::Module &machine_module) {
    for (const std::string &external : machine_module.get_external_symbols()) {
        add_symbol_def(SymbolDef{.name = external, .kind = BinSymbolKind::UNKNOWN, .global = true});
    }

    std::uint32_t first_text_symbol_index = defs.size();

    for (mcode::Function *func : machine_module.get_functions()) {
        add_func_symbol(func->get_name(), machine_module);

        for (mcode::BasicBlock &block : func->get_basic_blocks()) {
            if (!block.get_label().empty()) {
                add_label_symbol(block.get_label());
            }
        }

        add_label_symbol("");
    }

    data_slices.push_back(DataSlice{});

    for (const mcode::Global &global : machine_module.get_globals()) {
        add_data_symbol(global.get_name(), machine_module);

        if (global.get_value().is_data()) {
            for (char c : global.get_value().get_data()) {
                data_slices.back().buffer.write_u8(c);
            }
        } else if (global.get_value().is_immediate()) {
            std::string str = global.get_value().get_immediate();
            if (str.find('.') == std::string::npos) {
                if (global.get_value().get_size() == 1) {
                    data_slices.back().buffer.write_i8(std::stoll(str));
                } else if (global.get_value().get_size() == 2) {
                    data_slices.back().buffer.write_i16(std::stoll(str));
                } else if (global.get_value().get_size() == 4) {
                    data_slices.back().buffer.write_i32(std::stoll(str));
                } else if (global.get_value().get_size() == 8) {
                    data_slices.back().buffer.write_i64(std::stoll(str));
                }
            } else {
                // TODO: fix this in the lowerer
                str = str.substr(12, str.length() - 13);

                if (global.get_value().get_size() == 4) {
                    data_slices.back().buffer.write_f32(std::stof(str));
                } else if (global.get_value().get_size() == 8) {
                    data_slices.back().buffer.write_f64(std::stod(str));
                }
            }
        } else if (global.get_value().is_symbol()) {
            add_data_symbol_use(global.get_value().get_symbol().name);
            data_slices.back().buffer.write_i64(0);
        }
    }

    std::uint32_t symbol_index = first_text_symbol_index;
    create_text_slice();

    for (mcode::Function *func : machine_module.get_functions()) {
        UnwindInfo frame_info{.start_symbol_index = symbol_index, .alloca_size = func->get_unwind_info().alloc_size};

        attach_symbol_def(symbol_index++);

        for (mcode::BasicBlock &block : func->get_basic_blocks()) {
            if (!block.get_label().empty()) {
                attach_symbol_def(symbol_index++);
            }

            for (mcode::Instruction &instr : block) {
                encode_instr(instr, func, frame_info);
            }
        }

        frame_info.end_symbol_index = symbol_index;
        attach_symbol_def(symbol_index++);

        unwind_info.push_back(frame_info);
    }

    compute_slice_offsets();
    apply_relaxation();
    remove_internal_symbols();

    return create_module();
}

void X8664Encoder::apply_relaxation() {
    bool continue_ = true;

    for (unsigned pass = 0; pass < MAX_RELAXATION_PASSES && continue_; pass++) {
        continue_ = false;

        for (unsigned i = 0; i < text_slices.size(); i++) {
            DataSlice &slice = text_slices[i];
            if (!slice.relaxable_branch) {
                continue;
            }

            SymbolUse &use = slice.uses[0];
            std::uint8_t opcode = slice.buffer.get_data()[0];
            std::int32_t displacement = compute_branch_displacement(slice);

            if (fits_in_i8(displacement)) {
                slice.buffer.seek(use.local_offset);
                slice.buffer.write_i8(displacement);
                continue;
            }

            if (opcode == 0xEB) {
                continue_ = true;
                relax_jmp(i);
            } else if (opcode >= 0x72 && opcode <= 0x7E) {
                continue_ = true;
                relax_jcc(slice.uses[0], i);
            }
        }
    }
}

void X8664Encoder::remove_internal_symbols() {
    for (unsigned i = 0; i < text_slices.size(); i++) {
        DataSlice &slice = text_slices[i];
        if (!slice.relaxable_branch) {
            continue;
        }

        SymbolUse &use = slice.uses[0];
        std::int32_t displacement = compute_branch_displacement(slice);
        slice.buffer.seek(use.local_offset);

        if (fits_in_i8(displacement)) {
            slice.buffer.write_i8(displacement);
        } else {
            slice.buffer.write_i32(displacement);
        }
    }
}

void X8664Encoder::encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo &frame_info) {
    using namespace target::X8664Opcode;
    using namespace mcode::PseudoOpcode;

    if (instr.is_flag(mcode::Instruction::FLAG_ALLOCA)) {
        frame_info.alloca_start_label = add_empty_label();
    }

    switch (instr.get_opcode()) {
        case MOV: encode_mov(instr, func); break;
        case MOVSX: encode_movsx(instr, func); break;
        case MOVZX: encode_movzx(instr, func); break;
        case ADD: encode_add(instr, func); break;
        case SUB: encode_sub(instr, func); break;
        case AND: encode_and(instr, func); break;
        case OR: encode_or(instr, func); break;
        case XOR: encode_xor(instr, func); break;
        case SHL: encode_shl(instr, func); break;
        case SHR: encode_shr(instr, func); break;
        case CDQ: encode_cdq(instr); break;
        case CQO: encode_cqo(instr); break;
        case IMUL: encode_imul(instr, func); break;
        case IDIV: encode_idiv(instr, func); break;
        case JMP: encode_jmp(instr); break;
        case CMP: encode_cmp(instr, func); break;
        case JE: encode_je(instr); break;
        case JNE: encode_jne(instr); break;
        case JA: encode_ja(instr); break;
        case JAE: encode_jae(instr); break;
        case JB: encode_jb(instr); break;
        case JBE: encode_jbe(instr); break;
        case JG: encode_jg(instr); break;
        case JGE: encode_jge(instr); break;
        case JL: encode_jl(instr); break;
        case JLE: encode_jle(instr); break;
        case CMOVE: encode_cmove(instr, func); break;
        case CMOVNE: encode_cmovne(instr, func); break;
        case CMOVA: encode_cmova(instr, func); break;
        case CMOVAE: encode_cmovae(instr, func); break;
        case CMOVB: encode_cmovb(instr, func); break;
        case CMOVBE: encode_cmovbe(instr, func); break;
        case CMOVG: encode_cmovg(instr, func); break;
        case CMOVGE: encode_cmovge(instr, func); break;
        case CMOVL: encode_cmovl(instr, func); break;
        case CMOVLE: encode_cmovle(instr, func); break;
        case LEA: encode_lea(instr, func); break;
        case CALL: encode_call(instr, func); break;
        case RET: encode_ret(); break;
        case PUSH: encode_push(instr); break;
        case POP: encode_pop(instr); break;
        case MOVSS: encode_movss(instr, func); break;
        case MOVSD: encode_movsd(instr, func); break;
        case MOVAPS: encode_movaps(instr, func); break;
        case MOVUPS: encode_movups(instr, func); break;
        case ADDSS: encode_addss(instr, func); break;
        case ADDSD: encode_addsd(instr, func); break;
        case SUBSS: encode_subss(instr, func); break;
        case SUBSD: encode_subsd(instr, func); break;
        case MULSS: encode_mulss(instr, func); break;
        case MULSD: encode_mulsd(instr, func); break;
        case DIVSS: encode_divss(instr, func); break;
        case DIVSD: encode_divsd(instr, func); break;
        case XORPS: encode_xorps(instr, func); break;
        case XORPD: encode_xorpd(instr, func); break;
        case MINSS: encode_minss(instr, func); break;
        case MAXSS: encode_maxss(instr, func); break;
        case SQRTSS: encode_sqrtss(instr, func); break;
        case UCOMISS: encode_ucomiss(instr, func); break;
        case CVTSS2SD: encode_cvtss2sd(instr, func); break;
        case CVTSD2SS: encode_cvtsd2ss(instr, func); break;
        case CVTSI2SS: encode_cvtsi2ss(instr, func); break;
        case CVTSI2SD: encode_cvtsi2sd(instr, func); break;
        case CVTSS2SI: encode_cvtss2si(instr, func); break;
        case CVTSD2SI: encode_cvtsd2si(instr, func); break;
        case EH_PUSHREG: process_eh_pushreg(instr, frame_info); break;
        default: abort("unknown instruction (opcode " + std::to_string(instr.get_opcode()) + ")"); break;
    }

    if (instr.is_flag(mcode::Instruction::FLAG_ALLOCA)) {
        frame_info.alloca_end_label = add_empty_label();
    }
}

void X8664Encoder::encode_mov(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (is_reg(dst)) {
        if (is_reg(src)) emit_mov_rr(reg(dst), reg(src), dst.get_size());
        else if (is_imm(src)) emit_mov_ri(reg(dst), imm(src), dst.get_size());
        else if (is_addr(src)) emit_mov_rm(reg(dst), addr(src, func), dst.get_size());
    } else if (is_addr(dst)) {
        if (is_reg(src)) emit_mov_mr(addr(dst, func), reg(src), dst.get_size());
        else if (is_imm(src)) emit_mov_mi(addr(dst, func), imm(src), dst.get_size());
    }
}

void X8664Encoder::encode_movsx(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    RegCode dst_reg = reg(dst);
    RegOrAddr src_roa = roa(src, func);
    unsigned dst_size = dst.get_size();
    unsigned src_size = src.get_size();

    emit_16bit_prefix_if_required(dst_size);
    emit_rex_rroa(dst_size, dst_reg, src_roa);

    if (src_size == 1) {
        emit_opcode(0x0F);
        emit_opcode(0xBE);
    } else if (src_size == 2) {
        emit_opcode(0x0F);
        emit_opcode(0xBF);
    } else if (src_size == 4) {
        emit_opcode(0x63);
    }

    emit_modrm_sib(dst_reg, src_roa);
}

void X8664Encoder::encode_movzx(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    RegCode dst_reg = reg(dst);
    RegOrAddr src_roa = roa(src, func);
    unsigned dst_size = dst.get_size();
    unsigned src_size = src.get_size();

    emit_16bit_prefix_if_required(dst_size);
    emit_rex_rroa(dst_size, dst_reg, src_roa);
    emit_opcode(0x0F);
    emit_opcode(src_size == 1 ? 0xB6 : 0xB7);
    emit_modrm_sib(dst_reg, src_roa);
}

void X8664Encoder::encode_add(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {0, 0x04, 0x05, 0x80, 0x81, 0x83, 0x00, 0x01, 0x02, 0x03});
}

void X8664Encoder::encode_sub(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {5, 0x2C, 0x2D, 0x80, 0x81, 0x83, 0x28, 0x29, 0x2A, 0x2B});
}

void X8664Encoder::encode_imul(mcode::Instruction &instr, mcode::Function *func) {
    assert_condition(instr.get_operands().get_size() == 2, "imul must have two operands");

    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    assert_condition(is_reg(dst), "imul destination must be a register");

    if (is_reg(src)) emit_imul_rr(reg(dst), reg(src), dst.get_size());
    else if (is_addr(src)) emit_imul_rm(reg(dst), addr(src, func), dst.get_size());
    else if (is_imm(src)) emit_imul_rri(reg(dst), imm(src), dst.get_size());
}

void X8664Encoder::encode_idiv(mcode::Instruction &instr, mcode::Function *func) {
    RegOrAddr src = roa(instr.get_operand(0), func);
    int size = instr.get_operand(0).get_size();

    emit_16bit_prefix_if_required(size);
    emit_rex_rroa(size, 0, src);
    emit_opcode(size == 1 ? 0xF6 : 0xF7);
    emit_modrm_sib(7, src);
}

void X8664Encoder::encode_and(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {4, 0x24, 0x25, 0x80, 0x81, 0x83, 0x20, 0x21, 0x22, 0x23});
}

void X8664Encoder::encode_or(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {1, 0x0C, 0x0D, 0x80, 0x81, 0x83, 0x08, 0x09, 0x0A, 0x0B});
}

void X8664Encoder::encode_xor(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {6, 0x34, 0x35, 0x80, 0x81, 0x83, 0x30, 0x31, 0x32, 0x33});
}

void X8664Encoder::encode_shl(mcode::Instruction &instr, mcode::Function *func) {
    encode_shift(instr, func, 4);
}

void X8664Encoder::encode_shr(mcode::Instruction &instr, mcode::Function *func) {
    encode_shift(instr, func, 5);
}

void X8664Encoder::encode_cdq(mcode::Instruction &instr) {
    emit_opcode(0x99);
}

void X8664Encoder::encode_cqo(mcode::Instruction &instr) {
    emit_rex(1, 0, 0, 0);
    emit_opcode(0x99);
}

void X8664Encoder::encode_jmp(mcode::Instruction &instr) {
    mcode::Operand &target = instr.get_operand(0);

    if (target.is_label()) {
        create_text_slice();
        get_text_slice().relaxable_branch = true;

        emit_opcode(0xEB);
        add_text_symbol_use(target.get_label(), 0);
        get_back_text_buffer().write_u8(0);

        create_text_slice();
    }
}

void X8664Encoder::encode_cmp(mcode::Instruction &instr, mcode::Function *func) {
    encode_basic_instr(instr, func, {7, 0x3C, 0x3D, 0x80, 0x81, 0x83, 0x38, 0x39, 0x3A, 0x3B});
}

void X8664Encoder::encode_je(mcode::Instruction &instr) {
    encode_jcc(instr, 0x74);
}

void X8664Encoder::encode_jne(mcode::Instruction &instr) {
    encode_jcc(instr, 0x75);
}

void X8664Encoder::encode_ja(mcode::Instruction &instr) {
    encode_jcc(instr, 0x77);
}

void X8664Encoder::encode_jae(mcode::Instruction &instr) {
    encode_jcc(instr, 0x73);
}

void X8664Encoder::encode_jb(mcode::Instruction &instr) {
    encode_jcc(instr, 0x72);
}

void X8664Encoder::encode_jbe(mcode::Instruction &instr) {
    encode_jcc(instr, 0x76);
}

void X8664Encoder::encode_jg(mcode::Instruction &instr) {
    encode_jcc(instr, 0x7F);
}

void X8664Encoder::encode_jge(mcode::Instruction &instr) {
    encode_jcc(instr, 0x7D);
}

void X8664Encoder::encode_jl(mcode::Instruction &instr) {
    encode_jcc(instr, 0x7C);
}

void X8664Encoder::encode_jle(mcode::Instruction &instr) {
    encode_jcc(instr, 0x7E);
}

void X8664Encoder::encode_cmove(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x44);
}

void X8664Encoder::encode_cmovne(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x45);
}

void X8664Encoder::encode_cmova(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x47);
}

void X8664Encoder::encode_cmovae(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x43);
}

void X8664Encoder::encode_cmovb(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x42);
}

void X8664Encoder::encode_cmovbe(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x46);
}

void X8664Encoder::encode_cmovg(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x4F);
}

void X8664Encoder::encode_cmovge(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x4D);
}

void X8664Encoder::encode_cmovl(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x4C);
}

void X8664Encoder::encode_cmovle(mcode::Instruction &instr, mcode::Function *func) {
    encode_cmovcc(instr, func, 0x4E);
}

void X8664Encoder::encode_lea(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    emit_lea_rm(reg(dst), addr(src, func), dst.get_size());
}

void X8664Encoder::encode_call(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &target = instr.get_operand(0);

    if (target.is_symbol()) {
        emit_opcode(0xE8);
        add_text_symbol_use(target.get_symbol().name, 0);
        get_back_text_buffer().write_i32(0);
    } else if (is_reg(target)) {
        assert_condition(target.get_size() == 8, "call target register must be a 64-bit register");
        emit_opcode(0xFF);
        emit_modrm_rr(2, reg(target));
    } else if (is_addr(target)) {
        Address a = addr(target, func);
        emit_rex_rm(0, 0, a);
        emit_opcode(0xFF);
        emit_mem_digit(a, 2);
    } else {
        abort("invalid target for call instruction");
    }
}

void X8664Encoder::encode_ret() {
    emit_ret();
}

void X8664Encoder::encode_push(mcode::Instruction &instr) {
    mcode::Operand &src = instr.get_operand(0);
    assert_condition(is_reg(src), "push source must be a register");

    if (reg(src) >= R8) {
        emit_rex(0, 0, 0, 1);
    }
    emit_combined_opcode(0x50, reg(src));
}

void X8664Encoder::encode_pop(mcode::Instruction &instr) {
    mcode::Operand &dst = instr.get_operand(0);
    assert_condition(is_reg(dst), "pop destination must be a register");

    if (reg(dst) >= R8) {
        emit_rex(0, 0, 0, 1);
    }
    emit_combined_opcode(0x58, reg(dst));
}

void X8664Encoder::encode_movss(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (is_reg(dst)) {
        emit_sse(0xF3, 0x10, reg(dst), roa(src, func), 0);
    } else if (is_addr(dst)) {
        Address dst_addr = addr(dst, func);
        RegCode src_reg = reg(src);

        emit_opcode(0xF3);
        emit_rex_rm(0, src_reg, dst_addr);
        emit_opcode(0x0F);
        emit_opcode(0x11);
        emit_mem_reg(dst_addr, src_reg);
    }
}

void X8664Encoder::encode_movsd(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (is_reg(dst)) {
        emit_sse(0xF2, 0x10, reg(dst), roa(src, func), 0);
    } else if (is_addr(dst)) {
        Address dst_addr = addr(dst, func);
        RegCode src_reg = reg(src);

        emit_opcode(0xF2);
        emit_rex_rm(0, src_reg, dst_addr);
        emit_opcode(0x0F);
        emit_opcode(0x11);
        emit_mem_reg(dst_addr, src_reg);
    }
}

void X8664Encoder::encode_movaps(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (is_reg(dst)) {
        RegCode dst_reg = reg(dst);
        RegOrAddr src_roa = roa(src, func);

        emit_rex_rroa(0, dst_reg, src_roa);
        emit_opcode(0x0F);
        emit_opcode(0x28);
        emit_modrm_sib(dst_reg, src_roa);
    } else if (is_addr(dst)) {
        RegOrAddr dst_roa = roa(dst, func);
        RegCode src_reg = reg(src);

        emit_rex_rroa(0, src_reg, dst_roa);
        emit_opcode(0x0F);
        emit_opcode(0x29);
        emit_modrm_sib(src_reg, dst_roa);
    }
}

void X8664Encoder::encode_movups(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (is_reg(dst)) {
        RegCode dst_reg = reg(dst);
        RegOrAddr src_roa = roa(src, func);

        emit_rex_rroa(0, dst_reg, src_roa);
        emit_opcode(0x0F);
        emit_opcode(0x10);
        emit_modrm_sib(dst_reg, src_roa);
    } else if (is_addr(dst)) {
        RegOrAddr dst_roa = roa(dst, func);
        RegCode src_reg = reg(src);

        emit_rex_rroa(0, src_reg, dst_roa);
        emit_opcode(0x0F);
        emit_opcode(0x11);
        emit_modrm_sib(src_reg, dst_roa);
    }
}

void X8664Encoder::encode_addss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x58);
}

void X8664Encoder::encode_addsd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF2, 0x58);
}

void X8664Encoder::encode_subss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x5C);
}

void X8664Encoder::encode_subsd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF2, 0x5C);
}

void X8664Encoder::encode_mulss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x59);
}

void X8664Encoder::encode_mulsd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF2, 0x59);
}

void X8664Encoder::encode_divss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x5E);
}

void X8664Encoder::encode_divsd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF2, 0x5E);
}

void X8664Encoder::encode_xorps(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    assert_condition(is_reg(dst), "SSE instructions can only operate on registers");

    if (is_reg(src)) {
        emit_rex_rr(0, reg(dst), reg(src));
        emit_opcode(0x0F);
        emit_opcode(0x57);
        emit_modrm_rr(reg(dst), reg(src));
    } else if (is_addr(src)) {
        emit_rex_rm(0, reg(dst), addr(src, func));
        emit_opcode(0x0F);
        emit_opcode(0x57);
        emit_mem_reg(addr(src, func), reg(dst));
    }
}

void X8664Encoder::encode_xorpd(mcode::Instruction &instr, mcode::Function *func) {
    emit_opcode(0x66);
    encode_xorps(instr, func);
}

void X8664Encoder::encode_minss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x5D);
}

void X8664Encoder::encode_maxss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x5F);
}

void X8664Encoder::encode_sqrtss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_op(instr, func, 0xF3, 0x51);
}

void X8664Encoder::encode_ucomiss(mcode::Instruction &instr, mcode::Function *func) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    RegCode dst_r = reg(dst);
    RegOrAddr src_roa = roa(src, func);

    emit_rex_rroa(4, dst_r, src_roa);
    emit_opcode(0x0F);
    emit_opcode(0x2E);
    emit_modrm_sib(dst_r, src_roa);
}

void X8664Encoder::encode_cvtss2sd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF3, 0x5A, 0);
}

void X8664Encoder::encode_cvtsd2ss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF2, 0x5A, 0);
}

void X8664Encoder::encode_cvtsi2ss(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF3, 0x2A, 0);
}

void X8664Encoder::encode_cvtsi2sd(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF2, 0x2A, 8);
}

void X8664Encoder::encode_cvtss2si(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF3, 0x2D, 0);
}

void X8664Encoder::encode_cvtsd2si(mcode::Instruction &instr, mcode::Function *func) {
    encode_sse_cvt(instr, func, 0xF2, 0x2D, 8);
}

void X8664Encoder::encode_basic_instr(
    mcode::Instruction &instr,
    mcode::Function *func,
    const BasicInstrOpcodes &opcodes
) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);
    unsigned size = dst.get_size();

    emit_16bit_prefix_if_required(size);

    if (is_roa(dst) && is_imm(src)) {
        RegOrAddr dst_roa = roa(dst, func);
        Immediate src_imm = imm(src);

        bool is_imm8 = fits_in_i8(src_imm.value);

        // Use the specialized encoding if the destination is EAX and the encoding is actually smaller.
        if (is_reg(dst) && dst.get_physical_reg() == target::X8664Register::RAX && !(is_imm8 && size > 1)) {
            emit_rex_rr(size, RegCode::EAX, RegCode::EAX);
            emit_opcode(size == 1 ? opcodes.rax_imm8 : opcodes.rax_imm16);

            if (size == 1) get_back_text_buffer().write_i8(src_imm.value);
            else if (size == 2) get_back_text_buffer().write_i16(src_imm.value);
            else if (size == 4 || size == 8) get_back_text_buffer().write_i32(src_imm.value);

            return;
        }

        emit_rex_rroa(size, RegCode::EAX, dst_roa);

        if (is_imm8) {
            emit_opcode(size == 1 ? opcodes.rm8_imm8 : opcodes.rm16_imm8);
            emit_modrm_sib(opcodes.digit, dst_roa);
            get_back_text_buffer().write_i8(src_imm.value);
        } else {
            emit_opcode(opcodes.rm16_imm16);
            emit_modrm_sib(opcodes.digit, dst_roa);

            if (size == 2) get_back_text_buffer().write_i16(src_imm.value);
            else if (size == 4 || size == 8) get_back_text_buffer().write_i32(src_imm.value);
        }
    } else if (is_roa(dst) && is_reg(src)) {
        RegOrAddr dst_roa = roa(dst, func);
        RegCode src_reg = reg(src);

        emit_rex_rroa(size, src_reg, dst_roa);
        emit_opcode(size == 1 ? opcodes.rm8_r8 : opcodes.rm16_r16);
        emit_modrm_sib(src_reg, dst_roa);
    } else if (is_reg(dst) && is_roa(src)) {
        RegCode dst_reg = reg(dst);
        RegOrAddr src_roa = roa(src, func);

        emit_rex_rroa(size, dst_reg, src_roa);
        emit_opcode(size == 1 ? opcodes.r8_rm8 : opcodes.r16_rm16);
        emit_modrm_sib(dst_reg, src_roa);
    }
}

void X8664Encoder::encode_shift(mcode::Instruction &instr, mcode::Function *func, std::uint8_t digit) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);
    std::uint8_t size = dst.get_size();

    RegOrAddr dst_roa = roa(dst, func);

    emit_16bit_prefix_if_required(size);
    emit_rex_rroa(size, 0, dst_roa);

    if (is_reg(src)) {
        RegCode src_reg = reg(src);
        assert_condition(src_reg == RegCode::ECX, "shift: src reg must be ecx");
        emit_opcode(size == 1 ? 0xD2 : 0xD3);
        emit_modrm_sib(digit, dst_roa);
    } else if (is_imm(src)) {
        Immediate src_imm = imm(src);
        assert_condition(src_imm.symbol_index == -1, "shift: cannot shift by symbol");

        if (src_imm.value == -1) {
            emit_opcode(size == 1 ? 0xD0 : 0xD1);
            emit_modrm_sib(digit, dst_roa);
        } else {
            emit_opcode(size == 1 ? 0xC0 : 0xC1);
            emit_modrm_sib(digit, dst_roa);
            get_back_text_buffer().write_i8(src_imm.value);
        }
    }
}

void X8664Encoder::encode_jcc(mcode::Instruction &instr, std::uint8_t opcode) {
    mcode::Operand &target = instr.get_operand(0);

    if (target.is_label()) {
        create_text_slice();
        get_text_slice().relaxable_branch = true;

        emit_opcode(opcode);
        add_text_symbol_use(target.get_label(), 0);
        get_back_text_buffer().write_i8(0);

        create_text_slice();
    }
}

void X8664Encoder::encode_cmovcc(mcode::Instruction &instr, mcode::Function *func, std::uint8_t opcode) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    assert_condition(dst.get_size() != 1, "size of cmovcc must be 2, 4 or 8");
    emit_cmovcc(opcode, reg(dst), roa(src, func), dst.get_size());
}

void X8664Encoder::encode_sse_op(
    mcode::Instruction &instr,
    mcode::Function *func,
    std::uint8_t prefix,
    std::uint8_t opcode
) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    assert_condition(is_reg(dst), "SSE instructions can only operate on registers");
    emit_sse(prefix, opcode, reg(dst), roa(src, func), 0);
}

void X8664Encoder::encode_sse_cvt(
    mcode::Instruction &instr,
    mcode::Function *func,
    std::uint8_t prefix,
    std::uint8_t opcode,
    std::uint8_t size
) {
    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);
    emit_sse(prefix, opcode, reg(dst), roa(src, func), size);
}

void X8664Encoder::emit_mov_rr(RegCode dst, RegCode src, std::uint8_t size) {
    emit_basic_rr(0x88, 0x89, dst, src, size);
}

void X8664Encoder::emit_mov_ri(RegCode dst, Immediate imm, std::uint8_t size) {
    if (size == 8 && fits_in_32_bits(imm)) {
        size = 4;
    }

    emit_16bit_prefix_if_required(size);
    emit_rex_o(size, dst);
    emit_combined_opcode(size == 1 ? 0xB0 : 0xB8, dst);

    if (imm.symbol_index == -1) {
        if (size == 1) get_back_text_buffer().write_i8(imm.value);
        else if (size == 2) get_back_text_buffer().write_i16(imm.value);
        else if (size == 4) get_back_text_buffer().write_i32(imm.value);
        else if (size == 8) get_back_text_buffer().write_i64(imm.value);
    } else {
        add_text_symbol_use(imm.symbol_index, BinSymbolUseKind::ABS64, 0);
        get_back_text_buffer().write_u64(0);
    }
}

void X8664Encoder::emit_mov_rm(RegCode dst, Address src, std::uint8_t size) {
    emit_basic_rm(0x8A, 0x8B, dst, src, size);
}

void X8664Encoder::emit_mov_mr(Address dst, RegCode src, std::uint8_t size) {
    emit_basic_mr(0x88, 0x89, dst, src, size);
}

void X8664Encoder::emit_mov_mi(Address dst, Immediate imm, std::uint8_t size) {
    assert_condition(imm.symbol_index == -1, "64-bit symbol cannot be encoded here");

    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, 0, dst);
    emit_opcode(size == 1 ? 0xC6 : 0xC7);

    if (size == 8) {
        size = 4;
    }

    emit_mem_digit(dst, 0, size);

    if (size == 1) get_back_text_buffer().write_i8(imm.value);
    else if (size == 2) get_back_text_buffer().write_i16(imm.value);
    else if (size == 4) get_back_text_buffer().write_i32(imm.value);
}

void X8664Encoder::emit_imul_rr(RegCode dst, RegCode src, std::uint8_t size) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rr(size, dst, src);
    emit_opcode(0x0F);
    emit_opcode(0xAF);
    emit_modrm_rr(dst, src);
}

void X8664Encoder::emit_imul_rm(RegCode dst, Address src, std::uint8_t size) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, dst, src);
    emit_opcode(0x0F);
    emit_opcode(0xAF);
    emit_mem_reg(src, dst);
}

void X8664Encoder::emit_imul_rri(RegCode dst, Immediate imm, std::uint8_t size) {
    // TODO: optimization for 8-bit immediates

    assert_condition(size == 4 || size == 8, "imul_rri size must be 4 or 8");
    assert_condition(imm.symbol_index == -1, "64-bit symbol cannot be encoded here");

    emit_16bit_prefix_if_required(size);
    emit_rex_rr(size, dst, dst);
    emit_opcode(0x69);
    emit_modrm_rr(dst, dst);
    get_back_text_buffer().write_i32(imm.value);
}

void X8664Encoder::emit_lea_rm(RegCode dst, Address src, std::uint8_t size) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, dst, src);
    get_back_text_buffer().write_u8(0x8D);
    emit_mem_reg(src, dst);
}

void X8664Encoder::emit_ret() {
    get_back_text_buffer().write_u8(0xC3);
}

void X8664Encoder::emit_basic_rr(
    std::uint8_t opcode8,
    std::uint8_t opcode32,
    RegCode dst,
    RegCode src,
    std::uint8_t size
) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rr(size, src, dst);
    get_back_text_buffer().write_u8(size == 1 ? opcode8 : opcode32);
    emit_modrm_rr(src, dst);
}

void X8664Encoder::emit_basic_ri(
    std::uint8_t opcode8,
    std::uint8_t opcode32,
    std::uint8_t opcode_imm8,
    std::uint8_t modrm_reg_digit,
    RegCode dst,
    Immediate imm,
    std::uint8_t size
) {
    assert_condition(imm.symbol_index == -1, "64-bit symbol cannot be encoded here");

    emit_16bit_prefix_if_required(size);
    emit_rex_r(size, dst);

    if (size == 1) {
        emit_opcode(opcode8);
        emit_modrm_rr(modrm_reg_digit, dst);
        get_back_text_buffer().write_i8(imm.value);
    } else if (imm.value >= -128 && imm.value <= 127) {
        emit_opcode(opcode_imm8);
        emit_modrm_rr(modrm_reg_digit, dst);
        get_back_text_buffer().write_i8(imm.value);
    } else if (size == 2) {
        emit_opcode(opcode32);
        emit_modrm_rr(modrm_reg_digit, dst);
        get_back_text_buffer().write_i16(imm.value);
    } else if (size == 4 || size == 8) {
        emit_opcode(opcode32);
        emit_modrm_rr(modrm_reg_digit, dst);
        get_back_text_buffer().write_i32(imm.value);
    }
}

void X8664Encoder::emit_basic_rm(
    std::uint8_t opcode8,
    std::uint8_t opcode32,
    RegCode dst,
    Address src,
    std::uint8_t size
) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, dst, src);
    emit_opcode(size == 1 ? opcode8 : opcode32);
    emit_mem_reg(src, dst);
}

void X8664Encoder::emit_basic_mr(
    std::uint8_t opcode8,
    std::uint8_t opcode32,
    Address dst,
    RegCode src,
    std::uint8_t size
) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, src, dst);
    emit_opcode(size == 1 ? opcode8 : opcode32);
    emit_mem_reg(dst, src);
}

void X8664Encoder::emit_basic_mi(
    std::uint8_t opcode8,
    std::uint8_t opcode32,
    std::uint8_t opcode_imm8,
    std::uint8_t modrm_reg_digit,
    Address dst,
    Immediate imm,
    std::uint8_t size
) {
    assert_condition(imm.symbol_index == -1, "64-bit symbol cannot be encoded here");

    emit_16bit_prefix_if_required(size);
    emit_rex_rm(size, 0, dst);

    if (size == 1) {
        emit_opcode(opcode8);
        emit_mem_digit(dst, modrm_reg_digit, 1);
        get_back_text_buffer().write_i8(imm.value);
    } else if (imm.value >= -128 && imm.value <= 127) {
        emit_opcode(opcode_imm8);
        emit_mem_digit(dst, modrm_reg_digit, 1);
        get_back_text_buffer().write_i8(imm.value);
    } else if (size == 2) {
        emit_opcode(opcode32);
        emit_mem_digit(dst, modrm_reg_digit, 2);
        get_back_text_buffer().write_i16(imm.value);
    } else if (size == 4 || size == 8) {
        emit_opcode(opcode32);
        emit_mem_digit(dst, modrm_reg_digit, 4);
        get_back_text_buffer().write_i32(imm.value);
    }
}

void X8664Encoder::emit_cmovcc(std::uint8_t opcode, RegCode dst, RegOrAddr src, std::uint8_t size) {
    emit_16bit_prefix_if_required(size);
    emit_rex_rroa(size, dst, src);
    emit_opcode(0x0F);
    emit_opcode(opcode);
    emit_modrm_sib(dst, src);
}

void X8664Encoder::emit_sse(std::uint8_t prefix, std::uint8_t opcode, RegCode dst, RegOrAddr src, std::uint8_t size) {
    emit_opcode(prefix);
    emit_rex_rroa(size, dst, src);
    emit_opcode(0x0F);
    emit_opcode(opcode);
    emit_modrm_sib(dst, src);
}

void X8664Encoder::emit_opcode(std::uint8_t opcode) {
    get_back_text_buffer().write_u8(opcode);
}

void X8664Encoder::emit_mem_reg(Address addr, RegCode reg) {
    emit_mem_digit(addr, reg);
}

void X8664Encoder::emit_mem_digit(Address addr, std::uint8_t digit, std::uint32_t offset_to_next_instr) {
    if (!addr.symbol.empty()) {
        assert_condition(addr.base == AddrRegCode::ADDR_NO_BASE, "addresses with symbols must not have a base");
        assert_condition(addr.index == AddrRegCode::ADDR_NO_INDEX, "addresses with symbols must not have an index");

        emit_modrm(0b00, digit, 0b101);
        add_text_symbol_use(addr.symbol, -offset_to_next_instr);
        get_back_text_buffer().write_i32(0);
        return;
    }

    bool has_index = addr.index != AddrRegCode::ADDR_NO_INDEX;
    std::uint8_t base_trunc = addr.base & 0b111;

    if (addr.displacement == 0) {
        if (base_trunc == AddrRegCode::ADDR_EBP) {
            if (has_index) {
                emit_modrm(0b01, digit, 0b100);
                emit_sib(addr.scale, addr.index, addr.base);
            } else {
                emit_modrm(0b01, digit, 0b101);
            }

            get_back_text_buffer().write_u8(0);
        } else {
            if (has_index || base_trunc == AddrRegCode::ADDR_ESP) {
                emit_modrm(0b00, digit, 0b100);
                emit_sib(addr.scale, addr.index, addr.base);
            } else {
                emit_modrm(0b00, digit, addr.base);
            }
        }
    } else if (fits_in_i8(addr.displacement)) {
        if (has_index || base_trunc == AddrRegCode::ADDR_ESP) {
            emit_modrm(0b01, digit, 0b100);
            emit_sib(addr.scale, addr.index, addr.base);
        } else {
            emit_modrm(0b01, digit, addr.base);
        }

        get_back_text_buffer().write_i8(addr.displacement);
    } else {
        if (has_index || base_trunc == AddrRegCode::ADDR_ESP) {
            emit_modrm(0b10, digit, 0b100);
            emit_sib(addr.scale, addr.index, addr.base);
        } else {
            emit_modrm(0b10, digit, addr.base);
        }

        get_back_text_buffer().write_i32(addr.displacement);
    }
}

void X8664Encoder::emit_combined_opcode(std::uint8_t opcode, std::uint8_t reg) {
    get_back_text_buffer().write_u8(opcode | (reg & 0b111));
}

void X8664Encoder::emit_modrm_rr(std::uint8_t reg, RegCode rm) {
    emit_modrm(0b11, reg, rm);
}

void X8664Encoder::emit_modrm_sib(std::uint8_t reg, RegOrAddr roa) {
    if (std::holds_alternative<RegCode>(roa)) emit_modrm_rr(reg, std::get<RegCode>(roa));
    else if (std::holds_alternative<Address>(roa)) emit_mem_digit(std::get<Address>(roa), reg);
}

void X8664Encoder::emit_16bit_prefix_if_required(std::uint8_t size) {
    if (size == 2) {
        emit_16bit_prefix();
    }
}

void X8664Encoder::emit_rex_r(std::uint8_t size, RegCode rm) {
    bool w = size == 8;
    bool b = rm > RegCode::EDI;

    bool required = size == 1 && (rm & 0b111) > RegCode::EBX;
    if (required || w || b) {
        emit_rex(w, 0, 0, b);
    }
}

void X8664Encoder::emit_rex_rr(std::uint8_t size, std::uint8_t reg, RegCode rm) {
    bool w = size == 8;
    bool r = reg > RegCode::EDI;
    bool b = rm > RegCode::EDI;

    bool required = size == 1 && ((reg & 0b111) > RegCode::EBX || (rm & 0b111) > RegCode::EBX);
    if (required || w || r || b) {
        emit_rex(w, r, 0, b);
    }
}

void X8664Encoder::emit_rex_rm(std::uint8_t size, std::uint8_t reg, Address addr) {
    std::uint8_t index = addr.index;
    std::uint8_t base = addr.base;

    bool w = size == 8;
    bool r = reg > AddrRegCode::ADDR_EDI;
    bool x = index > AddrRegCode::ADDR_EDI;
    bool b = base > AddrRegCode::ADDR_EDI;

    // The REX prefix is sometimes required to differentiate e.g. between DH and SI.
    bool required = size == 1 && (reg & 0b111) > AddrRegCode::ADDR_EBX;

    if (required || w || r || x || b) {
        emit_rex(w, r, x, b);
    }
}

void X8664Encoder::emit_rex_rroa(std::uint8_t size, std::uint8_t reg, RegOrAddr roa) {
    if (std::holds_alternative<RegCode>(roa)) emit_rex_rr(size, reg, std::get<RegCode>(roa));
    else if (std::holds_alternative<Address>(roa)) emit_rex_rm(size, reg, std::get<Address>(roa));
}

void X8664Encoder::emit_rex_o(std::uint8_t size, std::uint8_t opcode_ext) {
    bool w = size == 8;
    bool b = opcode_ext > AddrRegCode::ADDR_EDI;

    // The REX prefix is sometimes required to differentiate e.g. between DH and SI.
    bool required = size == 1 && (opcode_ext & 0b111) > AddrRegCode::ADDR_EBX;

    if (required || w || b) {
        emit_rex(w, 0, 0, b);
    }
}

void X8664Encoder::emit_modrm(std::uint8_t mod, std::uint8_t reg, std::uint8_t rm) {
    get_back_text_buffer().write_u8(((mod & 0b11) << 6) | ((reg & 0b111) << 3) | (rm & 0b111));
}

void X8664Encoder::emit_sib(std::uint8_t scale, std::uint8_t index, std::uint8_t base) {
    std::uint8_t ss = 0;

    if (scale == 1) ss = 0b00;
    else if (scale == 2) ss = 0b01;
    else if (scale == 4) ss = 0b10;
    else if (scale == 8) ss = 0b11;
    else abort("scale must be 1, 2, 4 or 8");

    get_back_text_buffer().write_u8(((ss & 0b11) << 6) | ((index & 0b111) << 3) | (base & 0b111));
}

void X8664Encoder::emit_16bit_prefix() {
    get_back_text_buffer().write_u8(0x66);
}

void X8664Encoder::emit_rex(bool w, bool r, bool x, bool b) {
    std::uint8_t w_bit = w ? 1 : 0;
    std::uint8_t r_bit = r ? 1 : 0;
    std::uint8_t x_bit = x ? 1 : 0;
    std::uint8_t b_bit = b ? 1 : 0;
    get_back_text_buffer().write_u8(0b01000000 | (w_bit << 3) | (r_bit << 2) | (x_bit << 1) | b_bit);
}

X8664Encoder::RegCode X8664Encoder::reg(mcode::Operand &operand) {
    return reg(operand.get_register());
}

X8664Encoder::Immediate X8664Encoder::imm(mcode::Operand &operand) {
    if (operand.is_immediate()) {
        return Immediate{.value = std::stoll(operand.get_immediate()), .symbol_index = -1};
    } else if (operand.is_symbol()) {
        return Immediate{.value = 0, .symbol_index = symbol_indices[operand.get_symbol().name]};
    }

    assert(!"not an immediate");
    return {};
}

X8664Encoder::Address X8664Encoder::addr(mcode::Operand &operand, mcode::Function *func) {
    if (operand.is_stack_slot()) {
        return Address{
            .scale = 1,
            .index = AddrRegCode::ADDR_NO_INDEX,
            .base = AddrRegCode::ADDR_ESP,
            .displacement = func->get_stack_frame().get_stack_slot(operand.get_stack_slot()).get_offset(),
        };
    }

    if (operand.is_symbol_deref()) {
        return Address{
            .scale = 1,
            .index = AddrRegCode::ADDR_NO_INDEX,
            .base = AddrRegCode::ADDR_NO_BASE,
            .displacement = 0,
            .symbol = operand.get_deref_symbol().name,
        };
    }

    mcode::IndirectAddress &machine_addr = operand.get_addr();

    Address addr = {
        .scale = 1,
        .index = AddrRegCode::ADDR_NO_INDEX,
        .base = AddrRegCode::ADDR_NO_BASE,
        .displacement = 0,
    };

    if (machine_addr.get_base().is_physical_reg()) {
        addr.base = (AddrRegCode)reg(machine_addr.get_base());
    } else if (machine_addr.get_base().is_stack_slot()) {
        addr.base = AddrRegCode::ADDR_ESP;
        addr.displacement =
            func->get_stack_frame().get_stack_slot(machine_addr.get_base().get_stack_slot()).get_offset();
    } else {
        abort("virtual registers cannot be emitted");
    }

    if (!machine_addr.has_offset()) {
        return addr;
    } else if (machine_addr.has_reg_offset()) {
        addr.scale = (std::uint8_t)machine_addr.get_scale();
        addr.index = (AddrRegCode)reg(machine_addr.get_reg_offset());
    } else if (machine_addr.has_int_offset()) {
        addr.displacement += machine_addr.get_scale() * machine_addr.get_int_offset();
    }

    return addr;
}

X8664Encoder::RegOrAddr X8664Encoder::roa(mcode::Operand &operand, mcode::Function *func) {
    if (is_reg(operand)) return reg(operand);
    else if (is_addr(operand)) return addr(operand, func);
    else abort("expected register or address operand");
}

bool X8664Encoder::is_reg(mcode::Operand &operand) {
    return operand.is_physical_reg();
}

bool X8664Encoder::is_imm(mcode::Operand &operand) {
    return operand.is_immediate() || operand.is_symbol();
}

bool X8664Encoder::is_addr(mcode::Operand &operand) {
    return operand.is_addr() || operand.is_stack_slot() || operand.is_symbol_deref();
}

bool X8664Encoder::is_roa(mcode::Operand &operand) {
    return is_reg(operand) || is_addr(operand);
}

X8664Encoder::RegCode X8664Encoder::reg(mcode::Register reg) {
    switch (reg.get_physical_reg()) {
        case target::X8664Register::RAX: return RegCode::EAX;
        case target::X8664Register::RCX: return RegCode::ECX;
        case target::X8664Register::RDX: return RegCode::EDX;
        case target::X8664Register::RBX: return RegCode::EBX;
        case target::X8664Register::RSP: return RegCode::ESP;
        case target::X8664Register::RBP: return RegCode::EBP;
        case target::X8664Register::RSI: return RegCode::ESI;
        case target::X8664Register::RDI: return RegCode::EDI;
        case target::X8664Register::R8: return RegCode::R8;
        case target::X8664Register::R9: return RegCode::R9;
        case target::X8664Register::R10: return RegCode::R10;
        case target::X8664Register::R11: return RegCode::R11;
        case target::X8664Register::R12: return RegCode::R12;
        case target::X8664Register::R13: return RegCode::R13;
        case target::X8664Register::R14: return RegCode::R14;
        case target::X8664Register::R15: return RegCode::R15;
        case target::X8664Register::XMM0: return RegCode::XMM0;
        case target::X8664Register::XMM1: return RegCode::XMM1;
        case target::X8664Register::XMM2: return RegCode::XMM2;
        case target::X8664Register::XMM3: return RegCode::XMM3;
        case target::X8664Register::XMM4: return RegCode::XMM4;
        case target::X8664Register::XMM5: return RegCode::XMM5;
        case target::X8664Register::XMM6: return RegCode::XMM6;
        case target::X8664Register::XMM7: return RegCode::XMM7;
        case target::X8664Register::XMM8: return RegCode::XMM8;
        case target::X8664Register::XMM9: return RegCode::XMM9;
        case target::X8664Register::XMM10: return RegCode::XMM10;
        case target::X8664Register::XMM11: return RegCode::XMM11;
        case target::X8664Register::XMM12: return RegCode::XMM12;
        case target::X8664Register::XMM13: return RegCode::XMM13;
        case target::X8664Register::XMM14: return RegCode::XMM14;
        case target::X8664Register::XMM15: return RegCode::XMM15;
        default: return RegCode::EAX;
    }
}

std::int32_t X8664Encoder::compute_branch_displacement(DataSlice &branch_slice) {
    SymbolUse &use = branch_slice.uses[0];
    SymbolDef &def = defs[use.index];

    std::uint8_t opcode = branch_slice.buffer.get_data()[0];
    std::uint32_t size = opcode == 0x0F || opcode == 0xE9 ? 4 : 1;

    std::uint32_t use_offset = branch_slice.offset + use.local_offset + size;
    std::uint32_t def_offset = text_slices[def.slice_index].offset + def.local_offset;
    return def_offset - use_offset;
}

void X8664Encoder::process_eh_pushreg(mcode::Instruction &instr, UnwindInfo &frame_info) {
    frame_info.pushed_regs.push_back(
        PushedRegInfo{.reg = (unsigned)reg(instr.get_operand(0)), .end_label = add_empty_label()}
    );
}

void X8664Encoder::relax_jmp(std::uint32_t slice_index) {
    DataSlice &slice = text_slices[slice_index];
    slice.buffer.seek(0);
    slice.buffer.write_i8(0xE9);
    slice.buffer.write_i32(0);
    push_out_slices(slice_index + 1, 3);
}

void X8664Encoder::relax_jcc(SymbolUse &use, std::uint32_t slice_index) {
    DataSlice &slice = text_slices[slice_index];
    std::uint8_t opcode = slice.buffer.get_data()[0];
    slice.buffer.seek(0);
    slice.buffer.write_i8(0x0F);
    slice.buffer.write_i8(opcode + 0x10);
    slice.buffer.write_i32(0);
    push_out_slices(slice_index + 1, 4);
    use.local_offset += 1;
}

bool X8664Encoder::fits_in_i8(std::int64_t value) {
    return value >= std::numeric_limits<std::int8_t>::min() && value <= std::numeric_limits<std::int8_t>::max();
}

bool X8664Encoder::fits_in_32_bits(Immediate imm) {
    if (imm.symbol_index != -1) {
        return false;
    }

    std::int64_t value = imm.value;
    return value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max();
}
