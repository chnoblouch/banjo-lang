#include "diagnostics.hpp"

#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, std::vector<lang::sir::Module *> &mods) {
    for (lang::sir::Module *mod : mods) {
        publish_diagnostics(connection, workspace, *mod);
    }
}

void publish_diagnostics(Connection &connection, Workspace &workspace, lang::sir::Module &mod) {
    File *file = workspace.find_file(mod);
    if (!file) {
        return;
    }

    ModuleIndex *index = workspace.find_index(&mod);
    if (!index) {
        return;
    }

    JSONArray diagnostics;

    for (const lang::Report &report : index->reports) {
        const lang::SourceLocation &location = *report.get_message().location;
        std::string message = report.get_message().text;

        JSONObject diagnostic{
            {"range", ProtocolStructs::range_to_lsp(file->content, location.range)},
            {"severity", static_cast<unsigned>(ProtocolStructs::report_type_to_lsp(report.get_type()))},
            {"message", message},
        };

        if (!report.get_notes().empty()) {
            JSONArray related_information;

            for (const lang::ReportMessage &note : report.get_notes()) {
                File *note_file = workspace.find_file(note.location->path);
                if (!note_file) {
                    continue;
                }

                JSONObject location{
                    {"uri", URI::encode_from_path(note_file->fs_path)},
                    {"range", ProtocolStructs::range_to_lsp(note_file->content, note.location->range)}
                };

                related_information.add(JSONObject{
                    {"location", location},
                    {"message", note.text},
                });
            }

            diagnostic.add("relatedInformation", related_information);
        }

        diagnostics.add(diagnostic);
    }

    std::string uri = URI::encode_from_path(file->fs_path);
    JSONObject notification{{"uri", uri}, {"diagnostics", diagnostics}};
    connection.send_notification("textDocument/publishDiagnostics", notification);
}

} // namespace lsp

} // namespace banjo
