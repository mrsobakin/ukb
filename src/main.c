#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ukb.h"

#define _maybe_fprintf(...) if (!args.silent) { fprintf(__VA_ARGS__); };
#define info(msg, ...) _maybe_fprintf(stderr, "\033[33m" msg "\033[0m\n", ##__VA_ARGS__);
#define fatal(msg, ...) _maybe_fprintf(stderr, "\033[31m" msg "\033[0m\n", ##__VA_ARGS__); exit(1);

static struct {
    bool silent;
    const char* backend;
} args;

static const char* usage =
    "Usage: ukb [-h] [-s] [-b backend]\n"
    "\n"
    "  -h  show this help page\n"
    "  -s  be silent and don't print anything to stderr\n"
    "  -b  manually specify backend that should be used\n"
    "      it will be used even if it is reported as unavailable\n"
    "";


void parse_args(int argc, char **argv) {
    int opt;

    args.silent = false;
    args.backend = NULL;

    while ((opt = getopt(argc, argv, "shb:")) != -1) {
        switch (opt) {
            case 's':
                args.silent = true;
                break;

            case 'b':
                args.backend = optarg;
                break;

            case '?':
                fatal("Invalid option or missing argument");
                break;

            case 'h':
            default:
                fprintf(stderr, "%s", usage);
                exit(1);
                break;
        }
    }
}


void cb_print(const char* layout) {
    printf("%s\n", layout);
    fflush(stdout);
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    const ukb_backend_t *b;

    if (args.backend) {
        b = ukb_find(args.backend);
        if (!b) {
            fatal("ukb: backend was not found: %s", args.backend);
        }
    } else {
        b = ukb_find_available();
        if (!b) {
            fatal("ukb: no available backend was found");
        }
    }

    info("ukb: Using %s backend", ukb_backend_name(b));

    ukb_err_t err = ukb_backend_listen(b, cb_print);

    if (err.msg) {
        fatal("ukb: %s", err.msg);
    }
}
