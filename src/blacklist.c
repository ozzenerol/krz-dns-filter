#include "../include/blacklist.h"
#include "../include/log.h"

#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 256
#define COMMENT '#'

BLACKLIST *blacklist = NULL;

size_t blacklist_count(void) {
    return HASH_COUNT(blacklist);
}

void blacklist_add(const char *domain) {
    BLACKLIST *item = malloc(sizeof(BLACKLIST));
    if (item == NULL) {
        LOG_ERROR("Error while allocating memory for blacklist item: %d", errno);
        return;
    }
    
    item->name = strdup(domain);
    HASH_ADD_KEYPTR(hh, blacklist, item->name, strlen(item->name), item);
}

BLACKLIST *blacklist_contains(const char *domain) {
    BLACKLIST *item;
    HASH_FIND(hh, blacklist, domain, strlen(domain), item);
    return item;
}

int blacklist_load(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        LOG_ERROR("Error while opening file %s: %d. Please make sure your user has the right privileges", filepath, errno);
        return -1;
    }
    
    LOG_INFO("Starting to load blacklist from %s", filepath);

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (buffer[0] == '\0' || buffer[0] == COMMENT)
            continue;
        blacklist_add(buffer);
    }

    LOG_INFO("Loaded %zu items inside the blacklist", blacklist_count());

    fclose(file);

    return 0;
}
