#pragma once

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define UKB_OK() return (ukb_err_t) { .msg = NULL };
#define UKB_ERR(m) return (ukb_err_t) { .msg = m };
#define UKB_PROPAGATE(expr)              \
    {                                    \
        ukb_err_t err = expr;            \
        if (err.msg != NULL) return err; \
    };
