#ifndef LSP_URI_H
#define LSP_URI_H

#include <filesystem>
#include <string>

namespace banjo {

namespace lsp {

namespace URI {

std::string decode(std::string uri);
std::string encode(std::string uri);

std::filesystem::path decode_to_path(std::string uri);
std::string encode_from_path(std::filesystem::path path);

} // namespace URI

} // namespace lsp

} // namespace banjo

#endif
