#ifndef HEAP_ARRAY_H
#define HEAP_ARRAY_H

#include <vector>

namespace banjo {

template <typename T>
class HeapArray {

private:
    T *data;
    int size;

public:
    ~HeapArray() { delete[] data; }
};

} // namespace banjo

#endif
