#include "symbol_ref.hpp"

#include "symbol/constant.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/function.hpp"
#include "symbol/global_variable.hpp"
#include "symbol/local_variable.hpp"
#include "symbol/parameter.hpp"
#include "symbol/structure.hpp"
#include "symbol/union.hpp"
#include "symbol/use.hpp"

namespace lang {

SymbolRef::SymbolRef(Function *func) : kind(SymbolKind::FUNCTION), target(func), visibility(func->get_visibility()) {}

void SymbolRef::destroy() {
    if (link) {
        return;
    }

    switch (kind) {
        case SymbolKind::LOCAL: delete get_local(); break;
        case SymbolKind::PARAM: delete get_param(); break;
        case SymbolKind::GLOBAL: delete get_global(); break;
        case SymbolKind::UNION_CASE_FIELD: delete get_union_case_field(); break;
        case SymbolKind::USE: delete get_use(); break;

        case SymbolKind::GROUP: {
            SymbolGroup *group = get_group();
            for (SymbolRef symbol : group->symbols) {
                symbol.destroy();
            }

            delete group;
            break;
        }

        case SymbolKind::NONE:
        case SymbolKind::MODULE:
        case SymbolKind::FUNCTION:
        case SymbolKind::CONST:
        case SymbolKind::STRUCT:
        case SymbolKind::FIELD:
        case SymbolKind::ENUM:
        case SymbolKind::ENUM_VARIANT:
        case SymbolKind::UNION:
        case SymbolKind::UNION_CASE:
        case SymbolKind::TYPE_ALIAS:
        case SymbolKind::GENERIC_FUNC:
        case SymbolKind::GENERIC_STRUCT: break;
    }
}

SymbolRef SymbolRef::as_link() const {
    SymbolRef copy(*this);
    copy.link = true;
    copy.sub_module = false;
    return copy;
}

SymbolRef SymbolRef::as_sub_module() const {
    SymbolRef copy(*this);
    copy.link = true;
    copy.sub_module = true;
    return copy;
}

Symbol *SymbolRef::get_symbol() const {
    switch (kind) {
        case SymbolKind::NONE: return nullptr;
        case SymbolKind::MODULE: return nullptr;
        case SymbolKind::FUNCTION: return get_func(); break;
        case SymbolKind::LOCAL: return get_local(); break;
        case SymbolKind::PARAM: return get_param(); break;
        case SymbolKind::GLOBAL: return get_global(); break;
        case SymbolKind::CONST: return get_const(); break;
        case SymbolKind::STRUCT: return get_struct(); break;
        case SymbolKind::FIELD: return get_struct(); break;
        case SymbolKind::ENUM: return get_enum(); break;
        case SymbolKind::ENUM_VARIANT: return get_enum_variant(); break;
        case SymbolKind::UNION: return get_union(); break;
        case SymbolKind::UNION_CASE: return get_union_case(); break;
        case SymbolKind::UNION_CASE_FIELD: return get_union_case_field(); break;
        case SymbolKind::TYPE_ALIAS: return get_type_alias(); break;
        case SymbolKind::GENERIC_FUNC: return get_generic_func(); break;
        case SymbolKind::GENERIC_STRUCT: return get_generic_struct(); break;
        case SymbolKind::USE: return nullptr;
        case SymbolKind::GROUP: return nullptr;
    }
}

std::optional<SymbolRef> SymbolRef::resolve() const {
    if (kind == SymbolKind::USE) {
        if (get_use()->get_target().get_kind() == SymbolKind::NONE) {
            return {};
        }

        return get_use()->get_target().resolve();
    }

    return *this;
}

} // namespace lang
