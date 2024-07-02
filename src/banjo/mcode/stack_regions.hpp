#ifndef MCODE_STACK_REGIONS_H
#define MCODE_STACK_REGIONS_H

#include "mcode/stack_frame.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace mcode {

struct ImplicitStackRegion {
    int size;
    int saved_reg_size;
};

struct ArgStoreStackRegion {
    int size;
    std::unordered_map<StackSlotID, int> offsets;
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

} // namespace banjo

#endif
