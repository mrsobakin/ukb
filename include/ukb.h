#pragma once

#include <stdbool.h>
#include <stdlib.h>


typedef struct ukb_err {
    const char* msg;
} ukb_err_t;


typedef void (*ukb_layout_cb_t)(const char *layout);

typedef struct ukb_backend ukb_backend_t;


#ifdef UKB_BACKENDS_INTERNAL
struct ukb_backend {
    const char *name;
    bool (*can_use)(void);
    ukb_err_t (*listen)(ukb_layout_cb_t);
};
#endif


// Array of all registered backends.
extern const ukb_backend_t* ukb_backends[];

// Number of all registered backends.
extern size_t ukb_backends_number;


// Find first available keyboard backend.
// If no available backends were found, NULL is returned.
const ukb_backend_t* ukb_find_available(void);

// Find keyboard backend with the given name.
// Before using the backend, you must verify that it is currently available by calling its `can_use` function.
// If backend with the given name was not found, NULL is returned.
const ukb_backend_t* ukb_find(const char* name);


// Get backend name.
const char* ukb_backend_name(const ukb_backend_t*);

// Check whether this backend is available.
bool ukb_backend_can_use(const ukb_backend_t*);

// Listen for layout changes and report them to the given callback.
// Upon calling this function, current layout will be immediately reported to callback.
ukb_err_t ukb_backend_listen(const ukb_backend_t*, ukb_layout_cb_t);
