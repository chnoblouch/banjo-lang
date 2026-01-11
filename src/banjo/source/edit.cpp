#include "edit.hpp"
#include "banjo/source/text_range.hpp"

namespace banjo::lang {

EditList::EditList(SourceFile &file) : file{&file} {
    file_content = file.get_content();
}

void EditList::add_replace_edit(TextRange range, std::string replacement) {
    if (range.start != range.end) {
        if (file_content.substr(range.start, range.end - range.start) == replacement) {
            return;
        }
    }

    edits.push_back(Edit{.range = range, .replacement = std::move(replacement)});
}

void EditList::add_replace_edit(TextRange dst_range, TextRange src_range) {
    if (dst_range == src_range) {
        return;
    }

    std::string replacement{file_content.substr(src_range.start, src_range.end - src_range.start)};
    edits.push_back(Edit{.range = dst_range, .replacement = std::move(replacement)});
}

void EditList::add_insert_edit(TextPosition position, std::string replacement) {
    edits.push_back(Edit{.range{position, position}, .replacement = std::move(replacement)});
}

void EditList::add_delete_edit(TextRange range) {
    edits.push_back(Edit{.range = range, .replacement = ""});
}

void EditList::apply_edits() {
    std::string output = apply_edits_to_source_copy();
    file->update_content(std::move(output));
}

std::string EditList::apply_edits_to_source_copy() {
    TextRange range{0, static_cast<unsigned>(file_content.size())};
    return apply_edits_to_source_copy(range);
}

std::string EditList::apply_edits_to_source_copy(TextRange range) {
    bool entire_file = range.start == 0 && range.end == file_content.size();

    std::string_view input;
    std::string output;

    if (entire_file) {
        input = file_content;
    } else {
        input = file_content.substr(range.start, range.end - range.start);
    }

    std::vector<Edit> edits_in_range;

    if (!entire_file) {
        for (Edit &edit : edits) {
            if (edit.range.start < range.start || edit.range.end > range.end) {
                continue;
            }

            TextPosition local_start = edit.range.start - range.start;
            TextPosition local_end = edit.range.end - range.start;
            edits_in_range.push_back(Edit{.range{local_start, local_end}, .replacement = edit.replacement});
        }
    }

    std::vector<Edit> &edits = entire_file ? this->edits : edits_in_range;

    std::sort(edits.begin(), edits.end(), [](const Edit &lhs, const Edit &rhs) {
        return lhs.range.start < rhs.range.start;
    });

    if (edits.empty()) {
        output = input;
    } else {
        for (unsigned i = 0; i < edits.size(); i++) {
            unsigned prev_end = i == 0 ? 0 : edits[i - 1].range.end;
            unsigned next_start = edits[i].range.start;
            std::string_view replacement = edits[i].replacement;

            output.insert(output.end(), input.begin() + prev_end, input.begin() + next_start);
            output.insert(output.end(), replacement.begin(), replacement.end());
        }

        unsigned last_end = edits.back().range.end;
        output.insert(output.end(), input.begin() + last_end, input.end());
    }

    return output;
}

} // namespace banjo::lang
