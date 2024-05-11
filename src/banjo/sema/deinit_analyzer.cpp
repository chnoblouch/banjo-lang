#include "deinit_analyzer.hpp"

#include "symbol/magic_functions.hpp"
#include "symbol/structure.hpp"

namespace lang {

DeinitAnalyzer::DeinitAnalyzer(SemanticAnalyzerContext &context) {}

void DeinitAnalyzer::analyze_local(ASTBlock *block, LocalVariable *local) {
    analyze_var(block, local, {local, local->get_data_type()});
}

void DeinitAnalyzer::analyze_param(ASTBlock *block, Parameter *param) {
    analyze_var(block, param, {param, param->get_data_type()});
}

void DeinitAnalyzer::analyze_var(ASTBlock *block, Variable *var, LocationElement location_element) {
    DataType *type = var->get_data_type();

    switch (type->get_kind()) {
        case DataType::Kind::STRUCT: create_struct_info(block, var, location_element); break;
        case DataType::Kind::UNION: create_union_info(block, var, location_element); break;
        default: break;
    }
}

void DeinitAnalyzer::create_struct_info(ASTBlock *block, Variable *var, LocationElement location_element) {
    Structure *struct_ = var->get_data_type()->get_structure();

    DeinitInfo &info = var->get_deinit_info();
    info.has_deinit = struct_->get_method_table().get_function(MagicFunctions::DEINIT);
    info.location.add_element(location_element);

    for (StructField *field : struct_->get_fields()) {
        create_member_info(info, field);
    }

    register_info_on_block(info, block);
}

void DeinitAnalyzer::create_member_info(DeinitInfo &info, StructField *field) {
    DataType *type = field->get_type();

    if (type->get_kind() != DataType::Kind::STRUCT) {
        info.member_info.push_back(DeinitInfo{.has_deinit = false});
        return;
    }

    Location member_location = info.location;
    member_location.add_element(LocationElement(field, type));

    bool member_has_deinit = type->get_structure()->get_method_table().get_function(MagicFunctions::DEINIT);
    unsigned index = info.member_info.size();
    info.member_info.push_back(DeinitInfo{.has_deinit = member_has_deinit, .location = member_location});

    for (StructField *sub_member : type->get_structure()->get_fields()) {
        create_member_info(info.member_info[index], sub_member);
    }

    if (field->is_no_deinit()) {
        info.member_info[index].set_unmanaged();
    }
}

void DeinitAnalyzer::create_union_info(ASTBlock *block, Variable* var, LocationElement location_element) {

}

void DeinitAnalyzer::register_info_on_block(DeinitInfo &info, ASTBlock *block) {
    if (info.has_deinit) {
        block->add_deinit_value(&info);
    }

    for (DeinitInfo &member_info : info.member_info) {
        register_info_on_block(member_info, block);
    }
}

} // namespace lang
