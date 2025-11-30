#include "formatter_util.hpp"

#include "banjo/format/formatter.hpp"
#include "banjo/source/source_file.hpp"

#include <algorithm>
#include <iostream>

namespace banjo::test {

void FormatterUtil::format(std::string source) {
    using namespace lang;

    SourceFile source_file{
        .mod_path{"test"},
        .sub_mod_paths{},
        .fs_path{"test.bnj"},
        .buffer{},
        .tokens{},
        .ast_mod = nullptr,
        .sir_mod = nullptr,
    };

    source_file.update_content(std::move(source));

    std::vector<Formatter::Edit> edits = Formatter{}.format(source_file);

    std::sort(edits.begin(), edits.end(), [](const Formatter::Edit &lhs, const Formatter::Edit &rhs) {
        return lhs.range.start < rhs.range.start;
    });

    std::string_view input = source_file.get_content();
    std::string output;

    for (unsigned i = 0; i < edits.size(); i++) {
        unsigned prev_end = i == 0 ? 0 : edits[i - 1].range.end;
        unsigned next_start = edits[i].range.start;
        std::string_view replacement = edits[i].replacement;

        output.insert(output.end(), input.begin() + prev_end, input.begin() + next_start);
        output.insert(output.end(), replacement.begin(), replacement.end());
    }

    if (edits.size() != 0) {
        unsigned last_end = edits.back().range.end;
        output.insert(output.end(), input.begin() + last_end, input.end());
    }

    std::cout << output;
}

} // namespace banjo::test
