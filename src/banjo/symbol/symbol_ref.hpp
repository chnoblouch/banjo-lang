#ifndef SYMBOL_REF_H
#define SYMBOL_REF_H

#include "symbol/generics.hpp"
#include "symbol/symbol.hpp"
#include "symbol/union.hpp"

#include <cassert>
#include <optional>
#include <vector>

namespace banjo {

namespace lang {

enum class SymbolKind {
    NONE,
    MODULE,
    FUNCTION,
    LOCAL,
    PARAM,
    GLOBAL,
    CONST,
    STRUCT,
    FIELD,
    ENUM,
    ENUM_VARIANT,
    UNION,
    UNION_CASE,
    UNION_CASE_FIELD,
    PROTO,
    TYPE_ALIAS,
    GENERIC_FUNC,
    GENERIC_STRUCT,
    USE,
    GROUP,
};

class ASTModule;
class Function;
class LocalVariable;
class Parameter;
class GlobalVariable;
class Constant;
class Structure;
class StructField;
class Enumeration;
class EnumVariant;
class Union;
class UnionCase;
class UnionCaseField;
class Protocol;
class TypeAlias;
class Use;
struct SymbolGroup;

class SymbolRef {

private:
    SymbolKind kind;
    void *target;
    SymbolVisbility visibility = SymbolVisbility::PUBLIC;

    bool link = false;
    bool sub_module = false;

public:
    SymbolRef() : kind(SymbolKind::NONE), target(nullptr) {}
    SymbolRef(ASTModule *module_) : kind(SymbolKind::MODULE), target(module_) { assert(target); }
    SymbolRef(Function *func);
    SymbolRef(LocalVariable *local) : kind(SymbolKind::LOCAL), target(local) {}
    SymbolRef(Parameter *param) : kind(SymbolKind::PARAM), target(param) {}
    SymbolRef(GlobalVariable *global) : kind(SymbolKind::GLOBAL), target(global) {}
    SymbolRef(Constant *const_) : kind(SymbolKind::CONST), target(const_) {}
    SymbolRef(Structure *struct_) : kind(SymbolKind::STRUCT), target(struct_) {}
    SymbolRef(StructField *field) : kind(SymbolKind::FIELD), target(field) {}
    SymbolRef(Enumeration *enum_) : kind(SymbolKind::ENUM), target(enum_) {}
    SymbolRef(EnumVariant *enum_variant) : kind(SymbolKind::ENUM_VARIANT), target(enum_variant) {}
    SymbolRef(Union *union_) : kind(SymbolKind::UNION), target(union_) {}
    SymbolRef(UnionCase *union_case) : kind(SymbolKind::UNION_CASE), target(union_case) {}
    SymbolRef(UnionCaseField *union_case_field) : kind(SymbolKind::UNION_CASE_FIELD), target(union_case_field) {}
    SymbolRef(Protocol *proto) : kind(SymbolKind::PROTO), target(proto) {}
    SymbolRef(TypeAlias *type_alias) : kind(SymbolKind::TYPE_ALIAS), target(type_alias) {}
    SymbolRef(GenericFunc *generic_func) : kind(SymbolKind::GENERIC_FUNC), target(generic_func) {}
    SymbolRef(GenericStruct *generic_struct) : kind(SymbolKind::GENERIC_STRUCT), target(generic_struct) {}
    SymbolRef(Use *use) : kind(SymbolKind::USE), target(use) {}
    SymbolRef(SymbolGroup *group) : kind(SymbolKind::GROUP), target(group) {}

    SymbolKind get_kind() const { return kind; }
    ASTModule *get_module() const { return static_cast<ASTModule *>(target); }
    Function *get_func() const { return static_cast<Function *>(target); }
    LocalVariable *get_local() const { return static_cast<LocalVariable *>(target); }
    Parameter *get_param() const { return static_cast<Parameter *>(target); }
    GlobalVariable *get_global() const { return static_cast<GlobalVariable *>(target); }
    Constant *get_const() const { return static_cast<Constant *>(target); }
    Structure *get_struct() const { return static_cast<Structure *>(target); }
    StructField *get_field() const { return static_cast<StructField *>(target); }
    Enumeration *get_enum() const { return static_cast<Enumeration *>(target); }
    EnumVariant *get_enum_variant() const { return static_cast<EnumVariant *>(target); }
    Union *get_union() const { return static_cast<Union *>(target); }
    UnionCase *get_union_case() const { return static_cast<UnionCase *>(target); }
    UnionCaseField *get_union_case_field() const { return static_cast<UnionCaseField *>(target); }
    Protocol *get_proto() const { return static_cast<Protocol*>(target); }
    TypeAlias *get_type_alias() const { return static_cast<TypeAlias *>(target); }
    GenericFunc *get_generic_func() const { return static_cast<GenericFunc *>(target); }
    GenericStruct *get_generic_struct() const { return static_cast<GenericStruct *>(target); }
    Use *get_use() const { return static_cast<Use *>(target); }
    SymbolGroup *get_group() const { return static_cast<SymbolGroup *>(target); }
    SymbolVisbility get_visibility() const { return visibility; }

    bool is_link() const { return link; }
    bool is_sub_module() const { return sub_module; }

    SymbolRef as_link() const;
    SymbolRef as_sub_module() const;

    Symbol *get_symbol() const;
    std::optional<SymbolRef> resolve() const;

    void destroy();
};

struct SymbolGroup {
    std::vector<SymbolRef> symbols;
};

} // namespace lang

} // namespace banjo

#endif
