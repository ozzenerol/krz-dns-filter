#include "../include/log.h"
#include "../include/blacklist.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        LOG_ERROR("Usage: main.c </path/to/blacklist.txt>");
        return EXIT_FAILURE;
    }

    if (blacklist_load(argv[1]) == -1) {
        LOG_ERROR("Failed to load the blacklist");
        return EXIT_FAILURE;
    }

    return 0;
}
