#ifndef MACROS_H
#define MACROS_H

#if DEBUG
#    include <cstdlib>
#    include <iostream>

#    define ASSERT_UNREACHABLE                                                                                         \
        {                                                                                                              \
            std::cerr << "unreachable code path triggered, aborting!\nat " << __FILE__ << ":" << __LINE__ << "\n"      \
                      << std::flush;                                                                                   \
            std::abort();                                                                                              \
        }
#else
#    ifdef _MSC_VER
#        define ASSERT_UNREACHABLE __assume(false)
#    else
#        define ASSERT_UNREACHABLE __builtin_unreachable()
#    endif
#endif

#endif
