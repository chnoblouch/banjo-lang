#include "diagnostics.hpp"

#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, const std::vector<lang::SourceFile *> &files) {
    for (lang::SourceFile *file : files) {
        publish_diagnostics(connection, workspace, *file);
    }
}

void publish_diagnostics(Connection &connection, Workspace &workspace, lang::SourceFile &file) {
    ModuleIndex *index = workspace.find_index(file.sir_mod);
    if (!index) {
        return;
    }

    JSONArray diagnostics;

    for (const lang::Report &report : index->reports) {
        const lang::SourceLocation &location = *report.get_message().location;
        std::string message = report.get_message().text;

        JSONObject diagnostic{
            {"range", ProtocolStructs::range_to_lsp(file.buffer, location.range)},
            {"severity", static_cast<unsigned>(ProtocolStructs::report_type_to_lsp(report.get_type()))},
            {"message", message},
        };

        if (!report.get_notes().empty()) {
            JSONArray related_information;

            for (const lang::ReportMessage &note : report.get_notes()) {
                lang::SourceFile *note_file = note.location->file;
                if (!note_file) {
                    continue;
                }

                JSONObject location{
                    {"uri", URI::encode_from_path(note_file->fs_path)},
                    {"range", ProtocolStructs::range_to_lsp(note_file->buffer, note.location->range)}
                };

                related_information.add(
                    JSONObject{
                        {"location", location},
                        {"message", note.text},
                    }
                );
            }

            diagnostic.add("relatedInformation", related_information);
        }

        diagnostics.add(diagnostic);
    }

    std::string uri = URI::encode_from_path(file.fs_path);
    JSONObject notification{{"uri", uri}, {"diagnostics", diagnostics}};
    connection.send_notification("textDocument/publishDiagnostics", notification);
}

} // namespace lsp

} // namespace banjo
