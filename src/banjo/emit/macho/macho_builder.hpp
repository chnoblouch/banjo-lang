#ifndef BANJO_EMIT_MACHO_MACHO_BUILDER
#define BANJO_EMIT_MACHO_MACHO_BUILDER

#include "banjo/emit/macho/macho_format.hpp"

namespace banjo {

class MachOBuilder {

public:
    MachOFile build();
};

} // namespace banjo

#endif
