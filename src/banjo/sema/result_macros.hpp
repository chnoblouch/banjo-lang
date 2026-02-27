#ifndef BANJO_SEMA_RESULT_MACROS_H
#define BANJO_SEMA_RESULT_MACROS_H

#define RESULT_MERGE(dst, expr)                                                                                        \
    {                                                                                                                  \
        Result partial_result = ((expr));                                                                              \
        if (partial_result != Result::SUCCESS) {                                                                       \
            dst = Result::ERROR;                                                                                       \
        }                                                                                                              \
    }

#define RESULT_RETURN_ON_ERROR(result)                                                                                 \
    if (((result)) != Result::SUCCESS) {                                                                               \
        return Result::ERROR;                                                                                          \
    }

#endif
