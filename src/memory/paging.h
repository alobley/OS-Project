#ifndef PAGING_H
#define PAGING_H

#include "alloc.h"
#include <util.h>
#include <string.h>
#include <memmanage.h>

// Generate the page directory and all tables for memory
PageDirectory* InitPaging(size_t totalMem);

#endif