#pragma once

#include "FBarragePrimitive.h"
// Name of table
 #define CUCKOO_TABLE_NAME SkelePrim
//
// Type of key
#define CUCKOO_KEY_TYPE FBarrageKey
//
// Type of mapped value
#define CUCKOO_MAPPED_TYPE FBLet
// ReSharper disable once CppUnusedIncludeDirective
//This, sadly, IS used, and this is the correct way to do it.
#include <libcuckoo-c/cuckoo_table_template.h>
