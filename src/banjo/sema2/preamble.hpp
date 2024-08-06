#ifndef PREAMBLE_H
#define PREAMBLE_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class Preamble {

private:
    SemanticAnalyzer &analyzer;

public:
    Preamble(SemanticAnalyzer &analyzer);
    void insert();

private:
    void insert_into(sir::Module &mod);
    sir::UseIdent *create_use_ident(const std::string &value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
