
set(FILES_CORE
	src/main.cpp
	src/core/async/delayTest.cpp
	src/core/container/Collection.cpp
   src/core/container/Queue.cpp
	src/core/debug.cpp
	src/core/maths/Matrix.cpp
	src/core/maths/Quaternion.cpp
	src/core/maths/Vector.cpp
	src/core/maths/util.cpp
	)

set(FILES_ENGINE
	src/main.cpp
	src/engine/console.cpp
	src/engine/gui/gui_access.hpp
	)

set(FILES_GAMEPLAY
	src/main.cpp
	)

set(FILES_UTILITY
	src/main.cpp
   src/utility/aggregation_allocator.cpp
   src/utility/algorithm.cpp
	src/utility/any.cpp
	src/utility/array.cpp
	src/utility/array_alloc.cpp
   src/utility/crypto/crc.cpp
	src/utility/iterator.cpp
   src/utility/null_allocator.cpp
	src/utility/optional.cpp
	src/utility/property.cpp
   src/utility/string_view.cpp
   src/utility/type_info.cpp
   src/utility/unicode.cpp
	src/utility/variant.cpp
	)
