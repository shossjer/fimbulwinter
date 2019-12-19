
set(FILES_CORE
	tst/main_core.cpp
	tst/core/async/delayTest.cpp
	tst/core/container/Collection.cpp
   tst/core/container/Queue.cpp
	tst/core/debug.cpp
	tst/core/maths/Matrix.cpp
	tst/core/maths/Quaternion.cpp
	tst/core/maths/Vector.cpp
	tst/core/maths/util.cpp
	)

set(FILES_ENGINE
	tst/main_core.cpp
	tst/engine/console.cpp
	tst/engine/gui/gui_access.hpp
	)

set(FILES_GAMEPLAY
	tst/main_core.cpp
	)

set(FILES_UTILITY
	tst/main.cpp
   tst/utility/aggregation_allocator.cpp
   tst/utility/algorithm.cpp
	tst/utility/any.cpp
	tst/utility/array.cpp
	tst/utility/array_alloc.cpp
   tst/utility/crypto/crc.cpp
	tst/utility/iterator.cpp
   tst/utility/null_allocator.cpp
	tst/utility/optional.cpp
	tst/utility/property.cpp
   tst/utility/string_view.cpp
   tst/utility/type_info.cpp
   tst/utility/unicode.cpp
	tst/utility/variant.cpp
	)
