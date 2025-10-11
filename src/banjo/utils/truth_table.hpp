#ifndef BANJO_UTILS_TRUTH_TABLE_H
#define BANJO_UTILS_TRUTH_TABLE_H

#include <bitset>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace banjo::utils {

template <typename Term>
class TruthTable {

public:
    typedef std::bitset<64> Input;

    struct Row {
        Input input;
        bool output;
    };

private:
    std::vector<Term> terms;
    std::vector<Row> rows;

public:
    TruthTable() {}

    explicit TruthTable(Term term) : terms{std::move(term)} {
        rows = {
            Row{.input = 0, .output = false},
            Row{.input = 1, .output = true},
        };
    }

private:
    TruthTable(std::vector<Term> terms, std::vector<Row> rows) : terms(std::move(terms)), rows(std::move(rows)) {}

public:
    static TruthTable merge_and(const TruthTable &a, const TruthTable &b) {
        if (a.rows.empty()) {
            return b;
        } else if (b.rows.empty()) {
            return a;
        }

        std::vector<Term> terms;
        terms.reserve(a.terms.size() + b.terms.size());

        terms.insert(terms.end(), a.terms.begin(), a.terms.end());
        terms.insert(terms.end(), b.terms.begin(), b.terms.end());

        std::vector<Row> rows;
        rows.reserve(a.terms.size() * b.terms.size());

        for (const Row &row_a : a.rows) {
            for (const Row &row_b : b.rows) {
                rows.push_back(Row{
                    .input = row_a.input | (row_b.input << a.terms.size()),
                    .output = row_a.output && row_b.output,
                });
            }
        }

        return TruthTable{terms, rows};
    }

    static TruthTable merge_or(const TruthTable &a, const TruthTable &b) {
        if (a.rows.empty()) {
            return b;
        } else if (b.rows.empty()) {
            return a;
        }

        std::vector<Term> terms;
        terms.reserve(a.terms.size() + b.terms.size());

        terms.insert(terms.end(), a.terms.begin(), a.terms.end());
        terms.insert(terms.end(), b.terms.begin(), b.terms.end());

        std::vector<Row> rows;
        rows.reserve(a.terms.size() * b.terms.size());

        for (const Row &row_a : a.rows) {
            for (const Row &row_b : b.rows) {
                rows.push_back(Row{
                    .input = row_a.input | (row_b.input << a.terms.size()),
                    .output = row_a.output || row_b.output,
                });
            }
        }

        return TruthTable{terms, rows};
    }

    static TruthTable merge_and(std::span<TruthTable> truth_tables) {
        TruthTable result;

        for (TruthTable truth_table : truth_tables) {
            result = merge_and(result, truth_table);
        }

        return result;
    }

    TruthTable negate() const {
        std::vector<Row> result_rows = rows;

        for (Row &result_row : result_rows) {
            result_row.output = !result_row.output;
        }

        return TruthTable{terms, result_rows};
    }

    bool is_subset_of(const TruthTable<Term> &other) const {
        if (terms.size() > other.terms.size()) {
            return false;
        }

        std::vector<unsigned> index_map(terms.size());

        for (unsigned term = 0; term < terms.size(); term++) {
            if (std::optional<unsigned> other_term = find_matching_term(terms[term], other)) {
                index_map[term] = *other_term;
            } else {
                return false;
            }
        }

        for (const Row &row_b : other.rows) {
            if (!row_b.output) {
                continue;
            }

            bool found_match = false;

            for (const Row &row_a : rows) {
                if (!row_a.output) {
                    continue;
                }

                bool inputs_match = true;

                for (unsigned term = 0; term < terms.size(); term++) {
                    if (row_a.input[term] != row_b.input[index_map[term]]) {
                        inputs_match = false;
                        break;
                    }
                }

                if (inputs_match) {
                    found_match = true;
                    break;
                }
            }

            if (!found_match) {
                return false;
            }
        }

        return true;
    }

private:
    std::optional<unsigned> find_matching_term(const Term &term, const TruthTable &other) const {
        for (unsigned i = 0; i < other.terms.size(); i++) {
            if (other.terms[i] == term) {
                return i;
            }
        }

        return {};
    }
};

} // namespace banjo::utils

#endif
