#include "initialize_handler.hpp"

#include "config/config.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

InitializeHandler::InitializeHandler() {}

InitializeHandler::~InitializeHandler() {}

JSONValue InitializeHandler::handle(const JSONObject &params, Connection &) {
    JSONValue workspace_folders = params.get("workspaceFolders");
    init_config(workspace_folders);

    return JSONObject{
        {"capabilities",
         JSONObject{
             {"textDocumentSync", JSONObject{{"openClose", true}, {"change", 1}, {"save", true}}},
             {"completionProvider",
              JSONObject{
                  {"triggerCharacters", JSONArray{"."}},
                  {"completionItem", JSONObject{{"labelDetailsSupport", true}}}
              }},
             {"definitionProvider", true},
             {"referencesProvider", true},
             {"renameProvider", true},
             {"semanticTokensProvider",
              JSONObject{
                  {"legend",
                   JSONObject{
                       {"tokenTypes",
                        JSONArray{"namespace",     "type",      "class",    "enum",     "interface",  "struct",
                                  "typeParameter", "parameter", "variable", "property", "enumMember", "event",
                                  "function",      "method",    "macro",    "keyword",  "modifier",   "comment",
                                  "string",        "number",    "regexp",   "operator", "decorator"}},
                       {"tokenModifiers",
                        JSONArray{
                            "declaration",
                            "definition",
                            "readonly",
                            "static",
                            "deprecated",
                            "abstract",
                            "async",
                            "modification",
                            "documentation",
                            "defaultLibrary"
                        }}
                   }},
                  {"full", true}
              }}
         }},
        {"serverInfo", JSONObject{{"name", "Banjo Language Server"}, {"version", "1.0"}}}
    };
}

void InitializeHandler::init_config(const JSONValue &workspace_folders) {
    if (workspace_folders.is_null()) {
        return;
    }

    const JSONArray &roots = workspace_folders.as_array();

    for (int i = 0; i < roots.length(); i++) {
        std::string uri = roots.get_object(i).get_string("uri");
        std::filesystem::path root_path = URI::decode_to_path(uri);

        lang::Config::instance().paths.push_back(root_path / "src");

        std::filesystem::path packages_path = root_path / "packages";

        if (std::filesystem::is_directory(packages_path)) {
            for (const std::filesystem::path &package_path : std::filesystem::directory_iterator(packages_path)) {
                lang::Config::instance().paths.push_back(package_path / "src");
            }
        }
    }
}

} // namespace lsp

} // namespace banjo
