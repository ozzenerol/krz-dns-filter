#ifndef BLACKLIST_H
#define BLACKLIST_H

#include "vendor/uthash.h"

typedef struct {
    char            *name;
    UT_hash_handle  hh;
} BLACKLIST;

extern BLACKLIST *blacklist;

/* Hash Table operations */
size_t      blacklist_count(void);
void        blacklist_add(const char *domain);
BLACKLIST   *blacklist_contains(const char *domain);

/* Filesystem loading operations */
int blacklist_load(const char *filepath);

#endif // BLACKLIST_H
