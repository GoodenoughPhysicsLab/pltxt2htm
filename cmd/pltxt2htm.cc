#include <cstddef>
#include <cstring>
#include <cassert>
#include <exception/exception.hh>
#include <pltxt2htm/pltxt2htm.hh>
#include <fast_io/fast_io.h>

int main(int argc, char const* const* const argv)
#if __cpp_exceptions < 199711L
    noexcept
#endif // __cpp_exceptions < 199711L
{
    if (argc == 1) {
        ::fast_io::print(
            "version: 0.0.0\n"
            "exception: "
#if __cpp_exceptions >= 199711L
            "enable\n"
#else
            "disable\n"
#endif
            "rtti: "
#if __cpp_rtti >= 199711L
            "enable\n"
#else
            "disable\n"
#endif
            "build mode: "
#ifdef NDEBUG
            "release\n"
#else
            "debug\n"
#endif
            // TODO get install path
        );
        return 0;
    }

    // store input file path
#ifdef NDEBUG
    #if __has_cpp_attribute(indeterminate)
    char const* input_file_path [[indeterminate]];
    #else
    char const* input_file_path;
    #endif
#else
    char const* input_file_path = nullptr;
#endif
    // store output file path, can be optional
    char const* output_file_path = nullptr;
    for (::std::size_t i{1}; i < static_cast<::std::size_t>(argc); ++i) {
        if (::std::strcmp(argv[i], "-i") == 0) {
            if (i == static_cast<::std::size_t>(argc) - 1) [[unlikely]] {
                ::fast_io::perrln("You must specify input file after `-i`");
                return 1;
            }
            input_file_path = argv[++i];
            continue;
        } else if (::std::strcmp(argv[i], "-o") == 0) {
            if (i == static_cast<::std::size_t>(argc) - 1) [[unlikely]] {
                ::fast_io::perrln("You must specify output file after `-o`");
                return 1;
            }
            output_file_path = argv[++i];
            continue;
        } else if (::std::strcmp(argv[i], "-h") == 0 || ::std::strcmp(argv[i], "--help") == 0) {
            if (i != 1) [[unlikely]] {
                ::fast_io::perrln(
                    "You can only use `pltxt2htm [-h|--help]` without another options to show "
                    "helps");
                return 1;
            }
            ::fast_io::println(R"(Usage:
    pltxt2htm help
    pltxt2htm -i <input file>
    pltxt2htm -i <input file> -o <output file>
)");
            return 0;
        } else if (::std::strcmp(argv[i], "-v") == 0 || ::std::strcmp(argv[i], "--version") == 0) {
            if (i != 1) [[unlikely]] {
                ::fast_io::perrln(
                    "You can only use `pltxt2htm [--version|-v]` without another options to show "
                    "version");
                return 1;
            }
            ::fast_io::println("pltxt2htm v0.0.0");
            return 0;
        } else [[unlikely]] {
            ::fast_io::perrln("Unknown option: ", ::fast_io::mnp::os_c_str(argv[i]));
            return 1;
        }
    }

    assert(input_file_path != nullptr);

#if __cpp_exceptions >= 199711L
    try
#endif // __cpp_exceptions >= 199711L
    {
        ::fast_io::native_file_loader loader(::fast_io::mnp::os_c_str(input_file_path));
        auto html = ::pltxt2htm::pltxt2html<
#ifdef NDEBUG
            true, true
#else
            false, false
#endif
            >(::fast_io::mnp::os_c_str(reinterpret_cast<char8_t const*>(loader.data())));
        if (output_file_path == nullptr) {
            ::fast_io::println(::fast_io::u8c_stdout(), html);
        } else {
            auto output_file = ::fast_io::native_file{::fast_io::mnp::os_c_str(output_file_path), ::fast_io::open_mode::out};
            auto output_file_handle = ::fast_io::u8native_io_observer{output_file.native_handle()};
            ::fast_io::println(output_file_handle, html);
        }
    }
#if __cpp_exceptions >= 199711L
    catch (::fast_io::error const& e) {
        ::fast_io::perrln(e);
        return 1;
    }
#endif // __cpp_exceptions >= 199711L

    return 0;
}
