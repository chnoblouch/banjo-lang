#include "diagnostics.hpp"

#include "ast_navigation.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, std::vector<lang::sir::Module *> &mods) {
    for (lang::sir::Module *mod : mods) {
        File *file = workspace.find_file(*mod);
        if (!file) {
            continue;
        }

        ModuleIndex *index = workspace.find_index(mod);
        if (!index) {
            continue;
        }

        publish_diagnostics(connection, *file, *index);
    }
}

void publish_diagnostics(Connection &connection, File &file, ModuleIndex &index) {
    JSONArray diagnostics;

    for (const lang::Report &report : index.reports) {
        const lang::SourceLocation &location = *report.get_message().location;

        LSPTextPosition start = ASTNavigation::pos_to_lsp(file.content, location.range.start);
        LSPTextPosition end = ASTNavigation::pos_to_lsp(file.content, location.range.end);

        std::string message = report.get_message().text;

        for (const lang::ReportMessage &note : report.get_notes()) {
            message += "\n" + note.text;
        }

        // TODO: add protocol enum
        int severity;
        switch (report.get_type()) {
            case lang::Report::Type::ERROR: severity = 1; break;
            case lang::Report::Type::WARNING: severity = 2; break;
        }

        diagnostics.add(JSONObject{
            {"range",
             JSONObject{
                 {"start", JSONObject{{"line", start.line}, {"character", start.column}}},
                 {"end", JSONObject{{"line", end.line}, {"character", end.column}}},
             }},
            {"severity", severity},
            {"message", message},
        });
    }

    std::string uri = URI::encode_from_path(file.fs_path);
    JSONObject notification{{"uri", uri}, {"diagnostics", diagnostics}};
    connection.send_notification("textDocument/publishDiagnostics", notification);
}

} // namespace lsp

} // namespace banjo
