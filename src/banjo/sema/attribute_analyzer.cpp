#include "attribute_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

AttributeAnalyzer::AttributeAnalyzer(SemanticAnalyzer &analyzer) : analyzer{analyzer} {}

void AttributeAnalyzer::analyze(sir::Attributes &attrs) {
    // TODO: Make sure attributes are used in the right context.

    for (sir::RawAttribute &raw_attr : attrs.raw_attrs) {
        analyze_raw_attr(attrs, raw_attr);
    }
}

void AttributeAnalyzer::analyze_raw_attr(sir::Attributes &attrs, sir::RawAttribute &raw_attr) {
    std::string_view name = raw_attr.name;
    std::string_view value = raw_attr.value;

    bool requires_value = false;
    bool has_value = !raw_attr.value.empty();

    if (name == "exposed") {
        attrs.exposed = true;
    } else if (name == "dllexport") {
        attrs.dllexport = true;
    } else if (name == "test") {
        attrs.test = true;
    } else if (name == "unmanaged") {
        attrs.unmanaged = true;
    } else if (name == "byval") {
        attrs.byval = true;
    } else if (name == "never_inline") {
        attrs.never_inline = true;
    } else if (name == "link_name") {
        requires_value = true;
        attrs.link_name = value;
    } else if (raw_attr.name == "layout") {
        requires_value = true;

        if (value == "default") {
            attrs.layout = sir::Attributes::Layout::DEFAULT;
        } else if (value == "packed") {
            attrs.layout = sir::Attributes::Layout::PACKED;
        } else if (value == "overlapping") {
            attrs.layout = sir::Attributes::Layout::OVERLAPPING;
        } else if (value == "c") {
            attrs.layout = sir::Attributes::Layout::C;
        } else if (has_value) {
            analyzer.report_generator.report_err_attr_invalid_layout(raw_attr);
        }
    } else {
        analyzer.report_generator.report_err_invalid_attr(raw_attr);
        return;
    }

    if (requires_value != has_value) {
        if (requires_value) {
            analyzer.report_generator.report_err_attr_missing_value(raw_attr);
        } else {
            analyzer.report_generator.report_err_attr_redundant_value(raw_attr);
        }
    }
}

} // namespace banjo::lang::sema
