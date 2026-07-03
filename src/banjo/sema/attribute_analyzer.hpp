#ifndef BANJO_SEMA_ATTRIBUTE_ANALYZER_H
#define BANJO_SEMA_ATTRIBUTE_ANALYZER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

class AttributeAnalyzer {

private:
    SemanticAnalyzer &analyzer;

public:
    AttributeAnalyzer(SemanticAnalyzer &analyzer);
    void analyze(sir::Attributes &attrs);

private:
    void analyze_raw_attr(sir::Attributes &attrs, sir::RawAttribute &raw_attr);    
};

} // namespace banjo::lang::sema

#endif
