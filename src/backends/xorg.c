#include <stdbool.h>
#include <string.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include "ukb.h"
#include "../utils.h"


static char current_layout[128] = {0};


static Display *x_display = 0;

static ukb_err_t update_current_layout() {
    XkbStateRec xkb_state;
    if (XkbGetState(x_display, XkbUseCoreKbd, &xkb_state) != Success) {
        UKB_ERR("Failed to get keyboard state.");
    }

    XkbDescPtr desc_ptr = XkbGetKeyboard(x_display, XkbAllComponentsMask, XkbUseCoreKbd);
    if (!desc_ptr) {
        UKB_ERR("Failed to get keyboard description.");
    }
    if (!desc_ptr->names) {
        UKB_ERR("Failed to get keyboard names.");
    }

    Status error_get_controls = XkbGetControls(x_display, XkbAllComponentsMask, desc_ptr);
    if (error_get_controls != Success || !desc_ptr->ctrls) {
        UKB_ERR("Failed to get keyboard controls.");
    }

    int num_groups = desc_ptr->ctrls->num_groups;
    if (xkb_state.group >= num_groups) {
        UKB_ERR("Group index out of range.");
    }

    char *name_ptr = XGetAtomName(x_display, desc_ptr->names->groups[xkb_state.group]);
    if (!name_ptr) {
        UKB_ERR("Failed to get group name.");
    }

    if (strncmp(current_layout, name_ptr, sizeof(current_layout)) != 0) {
        strncpy(current_layout, name_ptr, sizeof(current_layout) - 1);
    }

    XkbFreeKeyboard(desc_ptr, 0, True);
    XFree(name_ptr);

    UKB_OK();
}

static ukb_err_t xorg_wait_event() {
    Bool bret = XkbSelectEventDetails(x_display, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
    if (!bret) {
        UKB_ERR("XkbSelectEventDetails failed");
    }

    XEvent event;
    int iret = XNextEvent(x_display, &event);
    if (iret) {
        UKB_ERR("XNextEvent failed");
    }

    UKB_OK();
}

static ukb_err_t xorg_listen_loop(ukb_layout_cb_t cb) {
    while(1) {
        UKB_PROPAGATE(xorg_wait_event());
        UKB_PROPAGATE(update_current_layout());
        cb(current_layout);
    }
    UKB_OK();
}

static int xorg_open_display() {
    int event_code;
    int error_return;
    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;
    int reason_return;

    x_display = XkbOpenDisplay(NULL, &event_code, &error_return, &major, &minor, &reason_return);
    return reason_return;
}

/********************************* BACKEND ***********************************/

static bool xorg_can_use(void) {
    int reason_return = xorg_open_display();
    XCloseDisplay(x_display);
    return reason_return == XkbOD_Success;
}

static ukb_err_t xorg_listen(ukb_layout_cb_t cb) {
    int reason_return = xorg_open_display();

    switch (reason_return) {
    case XkbOD_Success:
        break;
    case XkbOD_BadLibraryVersion:
        UKB_ERR("Bad XKB library version.");
    case XkbOD_ConnectionRefused:
        UKB_ERR("Connection to X server refused.");
    case XkbOD_BadServerVersion:
        UKB_ERR("Bad X11 server version.");
    case XkbOD_NonXkbServer:
        UKB_ERR("XKB not present.");
    default:
        UKB_ERR("XKB refused to open the display.");
    }

    ukb_err_t err = xorg_listen_loop(cb);
    XCloseDisplay(x_display);
    return err;
}

const ukb_backend_t ukb_backend_xorg = {
    .name = "xorg",
    .can_use = xorg_can_use,
    .listen = xorg_listen,
};
