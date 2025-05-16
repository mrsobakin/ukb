#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#include "ukb.h"
#include "../utils.h"


#define POLL_DELAY 1


static HKL prev_layout = NULL;
static char current_layout[128] = {0};


static bool hkl_to_name(HKL hkl, char* dst, size_t n) {
    LANGID langid = LOWORD((DWORD_PTR)hkl);
    LCID lcid = MAKELCID(langid, SORT_DEFAULT);

    char name[LOCALE_NAME_MAX_LENGTH];

    if (GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE, name, LOCALE_NAME_MAX_LENGTH)) {
        strncpy(dst, name, n);
        return true;
    } else {
        return false;
    }
}

static HKL poll_layout_switch() {
    while(1) {
        HWND hwnd = GetForegroundWindow();
        DWORD thread_id = GetWindowThreadProcessId(hwnd, NULL);
        HKL new_layout = GetKeyboardLayout(thread_id);

        if (prev_layout != new_layout) {
            prev_layout = new_layout;
            return new_layout;
        }

        Sleep(POLL_DELAY);
    }
}

/********************************* BACKEND ***********************************/

static bool windows_can_use(void) {
    return true;
}

static ukb_err_t windows_listen(ukb_layout_cb_t cb) {
    while (1) {
        HKL layout = poll_layout_switch();
        if (!hkl_to_name(layout, current_layout, sizeof(current_layout) - 1)) {
            UKB_ERR("Unknown layout.");
        }
        cb(current_layout);
    }

    UKB_OK();
}

const ukb_backend_t ukb_backend_windows = {
    .name = "windows",
    .can_use = windows_can_use,
    .listen = windows_listen,
};
