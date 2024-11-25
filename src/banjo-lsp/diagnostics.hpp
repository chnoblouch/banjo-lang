#ifndef BANJO_LSP_DIAGNOSTICS_H
#define BANJO_LSP_DIAGNOSTICS_H

#include "connection.hpp"
#include "index.hpp"
#include "workspace.hpp"

namespace banjo {

namespace lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, std::vector<lang::sir::Module *> &mods);
void publish_diagnostics(Connection &connection, Workspace &workspace, lang::sir::Module &mod);

} // namespace lsp

} // namespace banjo

#endif
