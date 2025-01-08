#include "stack_frame_pass.hpp"

#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/mcode/stack_regions.hpp"
#include "banjo/utils/timing.hpp"

#include <algorithm>

namespace banjo {

namespace codegen {

StackFramePass::StackFramePass(target::TargetRegAnalyzer &analyzer) : analyzer(analyzer) {}

void StackFramePass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        run(func);
    }
}

void StackFramePass::run(mcode::Function *func) {
    PROFILE_SCOPE("stack frame builder");

    this->func = func;
    mcode::StackFrame &frame = func->get_stack_frame();
    mcode::CallingConvention *calling_conv = func->get_calling_conv();

    mcode::StackRegions regions;
    mcode::ImplicitStackRegion &implicit_region = regions.implicit_region;
    mcode::ArgStoreStackRegion &arg_store_region = regions.arg_store_region;
    mcode::GenericStackRegion &generic_region = regions.generic_region;
    std::unordered_map<int, int> pre_alloca_offsets;

    calling_conv->create_implicit_region(func, frame, regions);
    calling_conv->create_arg_store_region(frame, regions);

    pre_alloca_offsets.insert(arg_store_region.offsets.begin(), arg_store_region.offsets.end());

    int generic_bytes = 0;
    create_generic_region(generic_bytes, pre_alloca_offsets, arg_store_region.size);
    generic_region.size = generic_bytes;

    calling_conv->create_call_arg_region(func, frame, regions);

    int alloca_size = calling_conv->get_alloca_size(regions);
    int total_size = alloca_size + implicit_region.size;

    for (auto pre_alloca_offset : pre_alloca_offsets) {
        // Pre-alloca offsets are relative to the stack pointer
        // before the alloca instruction, so we have to add the
        // size to get the offset after the alloca instruction.

        int offset = pre_alloca_offset.second + alloca_size;
        frame.get_stack_slot(pre_alloca_offset.first).set_offset(offset);
    }

    frame.set_total_size(total_size);
    frame.set_size(alloca_size);

    for (mcode::BasicBlock &block : func->get_basic_blocks()) {
        for (mcode::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            iter = calling_conv->fix_up_instr(block, iter, analyzer);
        }
    }

    std::vector<ssa::Type> types;
    for (mcode::Parameter &param : func->get_parameters()) {
        types.push_back(param.type);
    }

    std::vector<mcode::ArgStorage> arg_storage = calling_conv->get_arg_storage(types);

    for (unsigned int i = 0; i < func->get_parameters().size(); i++) {
        mcode::Parameter &param = func->get_parameters()[i];
        if (arg_storage[i].in_reg) {
            continue;
        }

        mcode::StackSlot &slot = frame.get_stack_slot(std::get<mcode::StackSlotID>(param.storage));
        int sp_offset = arg_storage[i].stack_offset;
        slot.set_offset(frame.get_total_size() + sp_offset);
    }

    func->get_unwind_info().alloc_size = func->get_stack_frame().get_size();
}

void StackFramePass::create_generic_region(
    int &generic_region_size,
    std::unordered_map<int, int> &pre_alloca_offsets,
    int top
) {
    mcode::StackFrame &frame = func->get_stack_frame();

    int generic_slot_offset = top;

    for (int i = 0; i < frame.get_stack_slots().size(); i++) {
        mcode::StackSlot &slot = frame.get_stack_slots()[i];
        if (!slot.is_defined() && slot.get_type() == mcode::StackSlot::Type::GENERIC) {
            generic_slot_offset -= slot.get_size();
            pre_alloca_offsets.insert({i, generic_slot_offset});
        }
    }

    generic_region_size = -(generic_slot_offset + top);
}

} // namespace codegen

} // namespace banjo
