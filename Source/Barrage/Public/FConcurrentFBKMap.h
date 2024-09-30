#pragma once

#include "SkeletonTypes.h"
// Name of table
#define CUCKOO_TABLE_NAME KeyKey
//
// Type of key
#define CUCKOO_KEY_TYPE FSkeletonKey
//
// Type of mapped value
#define CUCKOO_MAPPED_TYPE FBarrageKey
// ReSharper disable once CppUnusedIncludeDirective
//This, sadly, IS used, and this is the correct way to do it.
#include <libcuckoo-c/cuckoo_table_template.h>
