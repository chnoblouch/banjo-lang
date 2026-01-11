#include "formatting_handler.hpp"

#include "banjo/format/formatter.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/utils/json.hpp"

#include "protocol_structs.hpp"
#include "uri.hpp"

namespace banjo::lsp {

using namespace lang;

FormattingHandler::FormattingHandler(Workspace &workspace) : workspace(workspace) {}

FormattingHandler::~FormattingHandler() {}

JSONValue FormattingHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    SourceFile *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    ReportManager report_manager;
    EditList edits = Formatter{report_manager, *file}.format();
    JSONArray lsp_edits;

    for (const Edit &edit : edits.get_elements()) {
        JSONObject lsp_edit{
            {"range", ProtocolStructs::range_to_lsp(file->get_content(), edit.range)},
            {"newText", edit.replacement},
        };

        lsp_edits.add(lsp_edit);
    }

    return lsp_edits;
}

} // namespace banjo::lsp
