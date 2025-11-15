#ifndef BANJO_LSP_DIAGNOSTICS_H
#define BANJO_LSP_DIAGNOSTICS_H

#include "banjo/ast/module_list.hpp"

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo {

namespace lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, const std::vector<lang::SourceFile *> &files);
void publish_diagnostics(Connection &connection, Workspace &workspace, lang::SourceFile &file);

} // namespace lsp

} // namespace banjo

#endif
