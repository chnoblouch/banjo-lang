#include "initialize_handler.hpp"

#include "banjo/config/config.hpp"
#include "uri.hpp"

namespace banjo::lsp {

InitializeHandler::InitializeHandler() {}

InitializeHandler::~InitializeHandler() {}

json::Value InitializeHandler::handle(const json::Object &params, Connection &) {
    json::Value workspace_folders = params.get("workspaceFolders");
    // init_config(workspace_folders);

    return json::Object{
        {"capabilities",
         json::Object{
             {"textDocumentSync", json::Object{{"openClose", true}, {"change", 1}, {"save", true}}},
             {"completionProvider",
              json::Object{
                  {"triggerCharacters", json::Array{"."}},
                  {"resolveProvider", true},
                  {"completionItem", json::Object{{"labelDetailsSupport", true}}}
              }},
             {"definitionProvider", true},
             {"referencesProvider", true},
             {"renameProvider", true},
             {"documentFormattingProvider", true},
             {"semanticTokensProvider",
              json::Object{
                  {"legend",
                   json::Object{
                       {"tokenTypes",
                        json::Array{"namespace",     "type",      "class",    "enum",     "interface",  "struct",
                                    "typeParameter", "parameter", "variable", "property", "enumMember", "event",
                                    "function",      "method",    "macro",    "keyword",  "modifier",   "comment",
                                    "string",        "number",    "regexp",   "operator", "decorator"}},
                       {"tokenModifiers",
                        json::Array{
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
        {"serverInfo", json::Object{{"name", "Banjo Language Server"}, {"version", "1.0"}}}
    };
}

void InitializeHandler::init_config(const json::Value &workspace_folders) {
    if (workspace_folders.is_null()) {
        return;
    }

    const json::Array &roots = workspace_folders.as_array();

    for (int i = 0; i < roots.length(); i++) {
        std::string uri = roots.get_object(i).get_string("uri");
        std::filesystem::path root_path = URI::decode_to_path(uri);

        Config::instance().paths.push_back(root_path / "src");

        std::filesystem::path packages_path = root_path / "packages";

        if (std::filesystem::is_directory(packages_path)) {
            for (const std::filesystem::path &package_path : std::filesystem::directory_iterator(packages_path)) {
                Config::instance().paths.push_back(package_path / "src");
            }
        }
    }
}

} // namespace banjo::lsp
