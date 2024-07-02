#ifndef CONFIG_STANDARD_LIB_H
#define CONFIG_STANDARD_LIB_H

#include <filesystem>

namespace banjo {

namespace lang {

class StandardLib {

public:
    static StandardLib &instance();

    void discover();
    std::filesystem::path get_path() { return path; }

private:
    std::filesystem::path path;
};

} // namespace lang

} // namespace banjo

#endif
