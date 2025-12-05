#ifndef BANJO_LSP_COMPLETION_ENGINE_H
#define BANJO_LSP_COMPLETION_ENGINE_H

#include "banjo/sir/sir.hpp"
#include "banjo/source/source_file.hpp"

namespace banjo::lsp {

class CompletionEngine {

public:
    struct CompletionItem {
        lang::SourceFile &file;
        lang::sir::Symbol symbol;
        bool requires_use;
    };

    lang::SourceFile *cur_file;
    std::vector<CompletionItem> cur_items;
};

} // namespace banjo::lsp

#endif
