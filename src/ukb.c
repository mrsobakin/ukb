#include <ukb.h>

#include <stdlib.h>
#include <string.h>

#include "utils.h"


#define DECLARE(backend) extern const ukb_backend_t backend;
#define REFERENCE(backend) &backend,

/********************************* BACKENDS **********************************/

#define UKB_BACKENDS(M) \
    M(ukb_backend_sway)

/*****************************************************************************/

UKB_BACKENDS(DECLARE)

const ukb_backend_t* ukb_backends[] = {
    UKB_BACKENDS(REFERENCE)
};

size_t ukb_backends_number = ARRAY_SIZE(ukb_backends);


const ukb_backend_t* ukb_find_available(void) {
    for (size_t i = 0; i < ukb_backends_number; i++) {
        if (ukb_backends[i]->can_use()) {
            return ukb_backends[i];
        }
    }

    return NULL;
}

const ukb_backend_t* ukb_find(const char* name) {
    for (size_t i = 0; i < ukb_backends_number; i++) {
        if (strcmp(ukb_backends[i]->name, name) == 0) {
            return ukb_backends[i];
        }
    }

    return NULL;
}


const char* ukb_backend_name(const ukb_backend_t *backend) {
    return backend->name;
}

bool ukb_backend_can_use(const ukb_backend_t *backend) {
    return backend->can_use();
}

ukb_err_t ukb_backend_listen(const ukb_backend_t *backend, ukb_layout_cb_t cb) {
    return backend->listen(cb);
}
