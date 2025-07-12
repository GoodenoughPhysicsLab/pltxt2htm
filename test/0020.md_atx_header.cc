#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include <pltxt2htm/pltxt2htm.hh>

int main() noexcept {
    auto html1 = ::pltxt2htm::pltxt2advanced_html(u8"\n# test<br>text", u8"_");
    auto answer1 = ::fast_io::u8string_view{u8"<br><h1>test</h1><br>text"};
    ::exception::assert_true(html1 == answer1);

    auto html2 = ::pltxt2htm::pltxt2advanced_html(u8"# test<br>text", u8"_");
    auto answer2 = ::fast_io::u8string_view{u8"<h1>test</h1><br>text"};
    ::exception::assert_true(html2 == answer2);

    auto html3 = ::pltxt2htm::pltxt2advanced_html(u8"# test\ntext", u8"_");
    auto answer3 = ::fast_io::u8string_view{u8"<h1>test</h1><br>text"};
    ::exception::assert_true(html3 == answer3);

    auto html4 = ::pltxt2htm::pltxt2advanced_html(u8"<br># test\ntext", u8"_");
    auto answer4 = ::fast_io::u8string_view{u8"<br><h1>test</h1><br>text"};
    ::exception::assert_true(html4 == answer4);

    return 0;
}
