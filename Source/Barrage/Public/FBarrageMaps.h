#pragma once

#include "SkeletonTypes.h"
#include "FBarragePrimitive.h"
#include "FBarrageKey.h"
// Do not use these types unless you know exactly what you are doing.
// Shallow Guard prevents the template files from being evaluated while things are unset.
// They aren't used while things are set anymore, but the guard format should be observed.
#define SHALLOW_GUARD_BARRAGE_MAPS
#include <libcuckoo/cuckoohash_map.hh>

typedef libcuckoo::cuckoohash_map<FSkeletonKey, FBarrageKey> KeyToKey;
typedef libcuckoo::cuckoohash_map<FBarrageKey, FBLet> KeyToFBLet;
#undef SHALLOW_GUARD_BARRAGE_MAPS