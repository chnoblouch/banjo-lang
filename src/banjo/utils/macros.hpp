#ifndef MACROS_H
#define MACROS_H

#if BANJO_DEBUG
#    include <cstdlib>  // IWYU pragma: export
#    include <iostream> // IWYU pragma: export

#    define ASSERT(condition)                                                                                          \
        if (!(condition)) {                                                                                            \
            std::cerr << "assertion failure, aborting!\nat " << __FILE__ << ":" << __LINE__ << "\n" << std::flush;     \
            std::abort();                                                                                              \
        }

#    define ASSERT_MESSAGE(condition, message)                                                                         \
        if (!(condition)) {                                                                                            \
            std::cerr << "assertion failure, aborting!\nmessage: " << message << "\nat " << __FILE__ << ":"            \
                      << __LINE__ << "\n"                                                                              \
                      << std::flush;                                                                                   \
            std::abort();                                                                                              \
        }

#    define ASSERT_UNREACHABLE                                                                                         \
        {                                                                                                              \
            std::cerr << "unreachable code path triggered, aborting!\nat " << __FILE__ << ":" << __LINE__ << "\n"      \
                      << std::flush;                                                                                   \
            std::abort();                                                                                              \
        }

#    define ASSUME(condition)                                                                                          \
        if (!(condition)) {                                                                                            \
            std::cerr << "assumption violated, aborting!\nat " << __FILE__ << ":" << __LINE__ << "\n" << std::flush;   \
            std::abort();                                                                                              \
        }
#else
#    define ASSERT(condition) ;
#    define ASSERT_MESSAGE(condition, message) ;
#    ifdef _MSC_VER
#        define ASSERT_UNREACHABLE __assume(false);
#        define ASSUME(condition) __assume(condition);
#    else
#        define ASSERT_UNREACHABLE __builtin_unreachable();
#        define ASSUME(condition)                                                                                      \
            if (!(condition)) __builtin_unreachable();
#    endif
#endif

#define EMPTY_BRANCH ;

#endif
