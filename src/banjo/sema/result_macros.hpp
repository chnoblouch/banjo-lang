#ifndef BANJO_SEMA_RESULT_MACROS_H
#define BANJO_SEMA_RESULT_MACROS_H

#define RESULT_MERGE(dst, expr)                                                                                        \
    {                                                                                                                  \
        if (((expr)) != Result::SUCCESS) {                                                                             \
            dst = Result::ERROR;                                                                                       \
        }                                                                                                              \
    }

#define RESULT_PROPAGATE(result)                                                                                       \
    if (((result)) != Result::SUCCESS) {                                                                               \
        return Result::ERROR;                                                                                          \
    }

#endif
