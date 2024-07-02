#include "structure.hpp"

#include "utils/macros.hpp"

namespace banjo {

namespace ir {

int Structure::get_member_index(const std::string &name) {
    for (int i = 0; i < members.size(); i++) {
        if (members[i].name == name) {
            return i;
        }
    }

    ASSERT_UNREACHABLE;
}

} // namespace ir

} // namespace banjo
