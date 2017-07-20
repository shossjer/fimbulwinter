
#ifndef UTILITY_PREPROCESSOR_HPP
#define UTILITY_PREPROCESSOR_HPP

#define PP_EXPAND_1(macro, n) macro((n))
#define PP_EXPAND_2(macro, n) PP_EXPAND_1(macro, (n)); PP_EXPAND_1(macro, (n) + 1)
#define PP_EXPAND_4(macro, n) PP_EXPAND_2(macro, (n)); PP_EXPAND_2(macro, (n) + 2)
#define PP_EXPAND_8(macro, n) PP_EXPAND_4(macro, (n)); PP_EXPAND_4(macro, (n) + 4)
#define PP_EXPAND_16(macro, n) PP_EXPAND_8(macro, (n)); PP_EXPAND_8(macro, (n) + 8)
#define PP_EXPAND_32(macro, n) PP_EXPAND_16(macro, (n)); PP_EXPAND_16(macro, (n) + 16)
#define PP_EXPAND_64(macro, n) PP_EXPAND_32(macro, (n)); PP_EXPAND_32(macro, (n) + 32)
#define PP_EXPAND_128(macro, n) PP_EXPAND_64(macro, (n)); PP_EXPAND_64(macro, (n) + 64)
#define PP_EXPAND_256(macro, n) PP_EXPAND_128(macro, (n)); PP_EXPAND_128(macro, (n) + 128)

#endif /* UTILITY_PREPROCESSOR_HPP */
