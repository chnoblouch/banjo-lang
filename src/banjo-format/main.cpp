#include "banjo/config/config.hpp"
#include "banjo/config/config_parser.hpp"
#include "banjo/format/formatter.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/reports/report_printer.hpp"
#include "banjo/source/source_file.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    std::string file_path = argv[argc - 1];
    banjo::lang::Config::instance() = banjo::lang::ConfigParser().parse(argc, argv);

    banjo::lang::ReportManager report_manager;

    std::ifstream stream{file_path, std::ios::binary};
    std::unique_ptr<banjo::lang::SourceFile> file = banjo::lang::SourceFile::read({}, file_path, stream);
    banjo::lang::Formatter{report_manager, *file}.format().apply_edits();

    if (report_manager.is_valid()) {
        std::ofstream{file_path, std::ios::binary} << file->get_content();
    }

    banjo::lang::ReportPrinter report_printer;

    if (banjo::lang::Config::instance().color_diagnostics) {
        report_printer.enable_colors();
    }

    report_printer.print_reports(report_manager.get_reports());

    return report_manager.is_valid() ? 0 : 1;
}
