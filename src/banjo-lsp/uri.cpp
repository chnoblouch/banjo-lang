#include "uri.hpp"

#include <sstream>

namespace lsp {

std::string URI::decode(std::string uri) {
    std::string result;

    for (int i = 0; i < uri.size(); i++) {
        if (uri[i] != '%') {
            result += uri[i];
        } else {
            std::string hex;
            hex += uri[++i];
            hex += uri[++i];
            result += (char)std::stoi(hex, nullptr, 16);
        }
    }

    return result;
}

std::string URI::encode(std::string uri) {
    std::stringstream result;

    for (char c : uri) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            result << c;
        } else {
            result << "%" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (int)c;
        }
    }

    return result.str();
}

std::filesystem::path URI::decode_to_path(std::string uri) {
    std::string decoded = decode(uri);
    std::string prefix = "file:///";
    int prefix_len = prefix.size();

    if (decoded.size() >= prefix_len && decoded.substr(0, prefix_len) == prefix) {
        return decoded.substr(prefix_len);
    } else {
        return "";
    }
}

std::string URI::encode_from_path(std::filesystem::path path) {
    std::string path_str = path.string();

    std::string lsp_path_str;
    lsp_path_str.resize(path_str.size());

    for (unsigned int i = 0; i < path_str.size(); i++) {
        char c = path_str[i];

        if (c == std::filesystem::path::preferred_separator) {
            lsp_path_str[i] = '/';
        } else {
            lsp_path_str[i] = c;
        }
    }

    return "file:///" + encode(lsp_path_str);
}

} // namespace lsp
