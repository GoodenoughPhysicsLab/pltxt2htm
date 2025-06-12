#include <pltxt2htm/pltxt2htm.h>

extern "C" char8_t const* advanced_parser(char8_t const* const text, char8_t const* const host) noexcept {
    return ::pltxt2htm::advanced_parser<
#ifdef NDEBUG
        true
#else
        false
#endif
        >(text, host);
}
