#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <json-c/json.h>

#include "ukb.h"
#include "../utils.h"


static char current_layout[128] = {0};

/********************************* SWAY IPC **********************************/

static const char SWAY_MAGIC[] = {'i', '3', '-', 'i', 'p', 'c'};
static const char INPUT_SUB_MSG[] = "[\"input\"]";

#define SWAY_IPC_SUBSCRIBE  0x2
#define SWAY_IPC_INPUT      0x80000015
#define SWAY_IPC_GET_INPUTS 100


typedef union header {
    uint8_t raw[14];
    #pragma pack(push, 1)
    struct fields {
        uint8_t magic[6];
        uint32_t length;
        uint32_t type;
    } fields;
    #pragma pack(pop)
} header_t;


static int sway_ipc_open(const char *socket_path) {
	int socketfd;
	if ((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return -1;
	}

	struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = {0},
    };
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        return -1;
	}

	return socketfd;
}

static bool sway_ipc_send(int fd, uint32_t type, uint32_t length, const char *data) {
    header_t h;
    memcpy(h.fields.magic, SWAY_MAGIC, 6);
    h.fields.length = length;
    h.fields.type = type;

    if (write(fd, h.raw, sizeof(header_t)) == -1) return false;
    if (write(fd, data, length) == -1) return false;

    return true;
}

static bool sway_ipc_recv(int fd, header_t *h, char **data) {
    uint32_t left = sizeof(header_t);
    uint8_t *dst = h->raw;
    ssize_t n;

    while (left) {
        if ((n = recv(fd, dst, left, 0)) == -1) {
            return false;
        }
        dst += n;
        left -= n;
    }

    *data = (char*)calloc(1, h->fields.length + 1);
    left = h->fields.length;
    dst = (uint8_t*)*data;

    while (left) {
        if ((n = recv(fd, dst, left, 0)) == -1) {
            free(*data);
            return false;
        }
        dst += n;
        left -= n;
    }

    return true;
}

static bool sway_update_layout_with_json(json_object *j) {
    const char *new_layout = json_object_get_string(json_object_object_get(j, "xkb_active_layout_name"));

    if (!new_layout) return false;

    if (strncmp(current_layout, new_layout, sizeof(current_layout)) != 0) {
        strncpy(current_layout, new_layout, sizeof(current_layout) - 1);
        return true;
    }

    return false;
}

static bool sway_update_layout_with_event(json_object *j) {
    const char *change = json_object_get_string(json_object_object_get(j, "change"));
    if (strcmp(change, "xkb_layout") != 0) return false;

    return sway_update_layout_with_json(json_object_object_get(j, "input"));
}

static ukb_err_t sway_init_layout(int fd) {
    if (!sway_ipc_send(fd, SWAY_IPC_GET_INPUTS, 0, NULL)) {
        UKB_ERR("layout init: ipc send error");
    }

    header_t h;
    char *payload;
    if (!sway_ipc_recv(fd, &h, &payload)) {
        UKB_ERR("layout init: ipc recv error");
    }

    json_object *j = json_tokener_parse(payload);

    int n = json_object_array_length(j);
    for (int i = 0; i < n; i++) {
        sway_update_layout_with_json(json_object_array_get_idx(j, i));
    }

    json_object_put(j);
    free(payload);

    UKB_OK();
}

/********************************* BACKEND ***********************************/

static bool sway_can_use(void) {
    return getenv("SWAYSOCK") != NULL;
}

static ukb_err_t sway_listen(ukb_layout_cb_t cb) {
    const char *swaysock = getenv("SWAYSOCK");
    if (swaysock == NULL) {
        UKB_ERR("listen: SWAYSOCK env is not defined");
    }

    int fd = sway_ipc_open(swaysock);
    if (fd == -1) {
        UKB_ERR("listen: ipc open error");
    }

    UKB_PROPAGATE(sway_init_layout(fd));

    cb(current_layout);

    sway_ipc_send(fd, SWAY_IPC_SUBSCRIBE, sizeof(INPUT_SUB_MSG), INPUT_SUB_MSG);

    while (1) {
        header_t h;
        char *payload;
        if (!sway_ipc_recv(fd, &h, &payload)) {
            UKB_ERR("listen: ipc recv error");
        }

        if (h.fields.type == SWAY_IPC_INPUT) {
            json_object *j = json_tokener_parse(payload);

            if (sway_update_layout_with_event(j)) {
                cb(current_layout);
            }

            json_object_put(j);
        }

        free(payload);
    }
}

const ukb_backend_t ukb_backend_sway = {
    .name = "sway",
    .can_use = sway_can_use,
    .listen = sway_listen,
};
