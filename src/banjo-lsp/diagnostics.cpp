#include "diagnostics.hpp"

#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo::lsp {

void publish_diagnostics(Connection &connection, Workspace &workspace, const std::vector<SourceFile *> &files) {
    for (SourceFile *file : files) {
        publish_diagnostics(connection, workspace, *file);
    }
}

void publish_diagnostics(Connection &connection, Workspace &workspace, SourceFile &file) {
    ModuleIndex *index = workspace.find_index(file.sir_mod);
    if (!index) {
        return;
    }

    json::Array diagnostics;

    for (const Report &report : index->reports) {
        const SourceLocation &location = *report.get_message().location;
        std::string message = report.get_message().text;

        json::Object diagnostic{
            {"range", ProtocolStructs::range_to_lsp(file.buffer, location.range)},
            {"severity", static_cast<unsigned>(ProtocolStructs::report_type_to_lsp(report.get_type()))},
            {"message", message},
        };

        if (!report.get_notes().empty()) {
            json::Array related_information;

            for (const ReportMessage &note : report.get_notes()) {
                SourceFile *note_file = note.location->file;
                if (!note_file) {
                    continue;
                }

                json::Object location{
                    {"uri", URI::encode_from_path(note_file->fs_path)},
                    {"range", ProtocolStructs::range_to_lsp(note_file->buffer, note.location->range)}
                };

                related_information.add(
                    json::Object{
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
    json::Object notification{{"uri", uri}, {"diagnostics", diagnostics}};
    connection.send_notification("textDocument/publishDiagnostics", notification);
}

} // namespace banjo::lsp
