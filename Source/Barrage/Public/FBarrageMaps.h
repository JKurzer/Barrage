#pragma once
// Do not use these types unless you know exactly what you are doing.
#define SHALLOW_GUARD_BARRAGE_MAPS

namespace wretched
{
	extern "C" {
		// ReSharper disable once CppUnusedIncludeDirective
#include "FConcurrentFBletMap.h"
	}
	//note the lack of a once guard in the template.cc.
	//yep. it works the way you think it does.
#include <libcuckoo-c/cuckoo_table_template.cc>
}
typedef wretched::SkelePrim FBarrageToFBlet;
#undef CUCKOO_TABLE_NAME
#undef CUCKOO_KEY_TYPE
#undef CUCKOO_MAPPED_TYPE

namespace OfTheEarth
{
	extern "C" {
		// ReSharper disable once CppUnusedIncludeDirective
#include "FConcurrentFBKMap.h"
	}
	//note the lack of a once guard in the template.cc.
	//yep. it works the way you think it does.
#include <libcuckoo-c/cuckoo_table_template.cc>
}
typedef OfTheEarth::KeyKey FSkeletonToBarrage;

#undef CUCKOO_TABLE_NAME
#undef CUCKOO_KEY_TYPE
#undef CUCKOO_MAPPED_TYPE
#undef SHALLOW_GUARD_BARRAGE_MAPS