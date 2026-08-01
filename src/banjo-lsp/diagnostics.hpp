#ifndef BANJO_LSP_DIAGNOSTICS_H
#define BANJO_LSP_DIAGNOSTICS_H

#include "banjo/ast/module_list.hpp"

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, const std::vector<SourceFile *> &files);
void publish_diagnostics(Connection &connection, Workspace &workspace, SourceFile &file);

} // namespace banjo::lsp

#endif
