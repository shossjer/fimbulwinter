#pragma once

// msvc has some issues expanding __VA_ARGS__ correctly, this macro
// can be used to fix that
#define PP_UNPACK(x) x
