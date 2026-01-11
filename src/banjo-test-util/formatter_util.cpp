#include "formatter_util.hpp"

#include "banjo/format/formatter.hpp"
#include "banjo/reports/report_manager.hpp"
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

    ReportManager report_manager;
    EditList edits = Formatter{report_manager, source_file}.format();
    std::cout << edits.apply_edits_to_source_copy();
}

} // namespace banjo::test
