#include "formatting_handler.hpp"

#include "banjo/format/formatter.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/utils/json.hpp"

#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo::lsp {

FormattingHandler::FormattingHandler(Workspace &workspace) : workspace(workspace) {}

FormattingHandler::~FormattingHandler() {}

json::Value FormattingHandler::handle(const json::Object &params, Connection & /*connection*/) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    SourceFile *file = workspace.find_file(fs_path);
    if (!file) {
        return json::Object{{"data", json::Array{}}};
    }

    ReportManager report_manager;
    EditList edits = Formatter{report_manager, *file}.format();
    json::Array lsp_edits;

    for (const Edit &edit : edits.get_elements()) {
        json::Object lsp_edit{
            {"range", ProtocolStructs::range_to_lsp(file->get_content(), edit.range)},
            {"newText", edit.replacement},
        };

        lsp_edits.add(lsp_edit);
    }

    return lsp_edits;
}

} // namespace banjo::lsp
