#ifndef MCODE_STACK_REGIONS_H
#define MCODE_STACK_REGIONS_H

#include <unordered_map>
#include <vector>

namespace mcode {

typedef int StackSlotIndex;

struct ImplicitStackRegion {
    int size;
    int saved_reg_size;
};

struct ArgStoreStackRegion {
    int size;
    std::unordered_map<StackSlotIndex, int> offsets;
};

struct GenericStackRegion {
    int size;
};

struct CallArgStackRegion {
    int size;
};

struct StackRegions {
    ImplicitStackRegion implicit_region;
    ArgStoreStackRegion arg_store_region;
    GenericStackRegion generic_region;
    CallArgStackRegion call_arg_region;
};

} // namespace mcode

#endif
