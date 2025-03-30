#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <string.h>

#include "../utils.h"
#include "ukb.h"

static char current_layout[128] = {0};

static Display *_display = 0;

static ukb_err_t update_current_layout() {
	XkbStateRec xkbState;
	if (XkbGetState(_display, XkbUseCoreKbd, &xkbState) != Success) {
		UKB_ERR("Failed to get keyboard state.");
	}

	XkbDescPtr descPtr = XkbGetKeyboard(_display, XkbAllComponentsMask, XkbUseCoreKbd);
	if (!descPtr) {
    	UKB_ERR("Failed to get keyboard description.");
  	}
	if (!descPtr->names) {
    	UKB_ERR("Failed to get keyboard names.");
	}

	Status ErrorGetControls = XkbGetControls(_display, XkbAllComponentsMask, descPtr);
	if (ErrorGetControls != Success || !descPtr->ctrls) {
    	UKB_ERR("Failed to get keyboard controls.");
	}

	int num_groups = descPtr->ctrls->num_groups;
	if (xkbState.group >= num_groups) {
    	UKB_ERR("Group index out of range.");
	}

	char *namePtr = XGetAtomName(_display, descPtr->names->groups[xkbState.group]);
	if (!namePtr) {
		UKB_ERR("Failed to get group name.");
	}

	if (strncmp(current_layout, namePtr, sizeof(current_layout)) != 0) {
		strncpy(current_layout, namePtr, sizeof(current_layout) - 1);
	}

	XkbFreeKeyboard(descPtr, 0, True);
	XFree(namePtr);

	UKB_OK();
}

static ukb_err_t xorg_wait_event() {

	Bool bret = XkbSelectEventDetails(_display, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
	if (!bret) {
		UKB_ERR("XkbSelectEventDetails failed")
	}

	XEvent event;
	int iret = XNextEvent(_display, &event);
	if (!iret) {
		UKB_ERR("XNextEvent failed");
	}

	UKB_OK();
}

/********************************* BACKEND ***********************************/

static bool xorg_can_use(void) {
	_display = XOpenDisplay(NULL);
	return _display != NULL;
}

static ukb_err_t xorg_listen(ukb_layout_cb_t cb) {
	char *displayName = strdup("");
	int eventCode;
	int errorReturn;
	int major = XkbMajorVersion;
	int minor = XkbMinorVersion;
	int reasonReturn;

	_display = XkbOpenDisplay(displayName, &eventCode, &errorReturn, &major, &minor, &reasonReturn);
	free(displayName);
  
	switch (reasonReturn) {
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

	while (1) {
		xorg_wait_event();
		update_current_layout();
		cb(current_layout);
	}
}

const ukb_backend_t ukb_backend_xorg = {
	.name = "xorg",
	.can_use = xorg_can_use,
	.listen = xorg_listen,
};