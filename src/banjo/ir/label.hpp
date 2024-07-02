#ifndef IR_LABEL_H
#define IR_LABEL_H

#include <string>

namespace banjo {

namespace ir {

class Label {

private:
    std::string name;
    int index;

public:
    Label(std::string name, int index) : name(name), index(index) {}
    std::string get_name() { return name; }
    int get_index() { return index; }
};

} // namespace ir

} // namespace banjo

#endif
