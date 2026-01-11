#ifndef BANJO_FORMAT_EDIT_H
#define BANJO_FORMAT_EDIT_H

#include "banjo/source/source_file.hpp"
#include "banjo/source/text_range.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace banjo::lang {

struct Edit {
    TextRange range;
    std::string replacement;
};

class EditList {

private:
    SourceFile *file;
    std::string_view file_content;
    std::vector<Edit> edits;

public:
    EditList(SourceFile &file);
    const std::vector<Edit> &get_elements() const { return edits; }

    void add_replace_edit(TextRange range, std::string replacement);
    void add_replace_edit(TextRange dst_range, TextRange src_range);
    void add_insert_edit(TextPosition position, std::string replacement);
    void add_delete_edit(TextRange range);
    void apply_edits();
    std::string apply_edits_to_source_copy();
    std::string apply_edits_to_source_copy(TextRange range);
};

} // namespace banjo::lang

#endif
