/**
 * @brief Macro defined by pltxt2htm
 * @note This header may be included multiple times with pop_macro.hh,
 *       therefore, do not `#pragma once`
 */

#ifdef PLTXT2HTM_ENABLE_PRINT
    #include "panic.hh"
#endif

#pragma push_macro("pltxt2htm_assert")
#undef pltxt2htm_assert
/**
 * @brief Assert whether the condition expression is true, if not, print
 *         the message and terminate the program.
 */
#ifdef PLTXT2HTM_ENABLE_PRINT
    #define pltxt2htm_assert(condition, message) \
        do { \
            if ((condition) == false) [[unlikely]] { \
                panic_print(); \
                if constexpr (ndebug == false) { \
                    ::exception::terminate(); \
                } \
            } \
        } while (0)
#else
    #define pltxt2htm_assert(condition, _) \
        do { \
            if constexpr (ndebug == false) { \
                if ((condition) == false) [[unlikely]] { \
                    ::exception::terminate(); \
                } \
            } \
        } while (0)
#endif
