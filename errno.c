#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static struct {
    const char *name;
    int code;
} errnos[] = {
    #include "errnos.h"
};
static const int num_errnos = sizeof(errnos) / sizeof(errnos[0]);


static void
report(const char *name, int code)
{
    printf("%s %d %s\n", name, code, strerror(code));
}


static bool
report_from_name(const char *name)
{
    int i;
    for (i = 0; i < num_errnos; ++i) {
        if (strcasecmp(errnos[i].name,  name) == 0) {
            report(errnos[i].name, errnos[i].code);
            return true;
        }
    }
    return false;
}


static bool
report_from_code(int code)
{
    int i;
    for (i = 0; i < num_errnos; ++i) {
        if (errnos[i].code == code) {
            report(errnos[i].name, code);
            return true;
        }
    }
    return false;
}


int 
main(int argc, char **argv)
{
    int i;
    int exit_code;
    
    exit_code = EXIT_SUCCESS;
    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (toupper(arg[0]) == 'E') {
            if (!report_from_name(arg))
                exit_code = EXIT_FAILURE;
        } else if (isdigit(arg[0])) {
            if (!report_from_code(atoi(arg)))
                exit_code = EXIT_FAILURE;
        } else {
            fprintf(stderr, "ERROR: Not understood: %s\n", arg);
            exit_code = EXIT_FAILURE;
        }
    }
    return exit_code;
}
