# pltxt2htm

![language](https://img.shields.io/badge/language-C++23-blue.svg)
![license](https://img.shields.io/badge/License-GPL-green.svg)

This repository is a sub-project of [Physics-Lab-Web](https://github.com/wsxiaolin/physics-lab-web)

This project aimed to provide a translater that compiles the Quantam-PhysicsLab's text to html (with markdown, minor html tags and latex support [as much as possible](#features)).

This project requires at least C++23, disables rtti and exceptions, use header-only style to supply api, has great cross platform ability and I strongly recommend you to use clang.

# How to use?
You can use `pltxt2htm` in [console](cmd/README.md), [browser](wasm/README.md) and [python](py/README.md).

C++ APIs is in [include/pltxt2htm](include/pltxt2htm/), here is a simple example:
```cpp
// example.cc
#include <fast_io/fast_io.h> // in include/fast_io
#include <pltxt2htm/pltxt2htm.hh> // in include/pltxt2htm

int main() noexcept {
    auto html = ::pltxt2htm::pltxt2html(u8R"(
# Hello Quantum PhysicsLab
With Markdown supports
)");
    ::fast_io::io::println(::fast_io::u8c_stdout, html);

    return 0;
}
```

Compile it: (Windows)
```sh
clang++ example.cc -o example -std=c++23 -I include -lntdll
```

Other platforms:
```sh
clang++ example.cc -o example -std=c++23 -I include
```

And I strongly suggest you to add `-O2`, `-fno-ident`, `-fno-exceptions`, `-fno-rtti`, `-fno-unwind-tables`, `-fno-asynchronous-unwind-tables`, `-DNDEBUG`, `-fuse-ld=lld`, `-flto=thin`, `-stdlib=libc++`, `-rtlib=compiler-rt`, `--unwindlib=libunwind` and cross compiling flags(`--target=$TRIPLET` and `--sysroot=$YOUR_SYSROOT_PATH`) to your clang.

## features
I am ensure any new features (like markdown extension) will break old Quantum Physics's text. However, compatibility is not the reason that we should stop our steps. Here are some features why I (or why I not) support:

markdown extension based on https://spec.commonmark.org/0.31.2/

* carriage return will be regarded as line break (unlike markdown, which requires 2 spaces before a carriage return to be considered a line break)
* HTML's br tag is supported (&lt;br&gt;, &lt;BR&gt;, &lt;br/&gt;, &lt;br&nbsp;&nbsp;&nbsp;&gt;, &lt;br&nbsp;&nbsp;&nbsp;/&gt; will be regarded as line break)
* Quantum-Physics's color tag is supported
* Quantum-Physics's a tag is supported
* Quantum-Physics's Discussion tag is supported
* Quantum-Physics's Experiment tag is supported
* Quantum-Physics&HTML's i tag is supported
* Quantum-Physics&HTML's b tag is supported
* HTML's <h1> tag is supported
* HTML's <h2> tag is supported
* HTML's <h3> tag is supported
* HTML's <h4> tag is supported
* HTML's <h5> tag is supported
* HTML's <h6> tag is supported
* HTML's <p> tag is supported

## others

> Q: Is this the time to use C++20 module?

A: Not exactly. Despite clang, gcc and msvc all support C++20 modules, but the compiler crashes more frequently than hearder-only. At the same time, Header unit is not fullly supported.

> Q: Why not use NDEBUG macro in include/pltxt2htm

A: Conditional compilation in function body will cause [ODR violation](https://en.cppreference.com/w/cpp/language/definition) and [C++26 Contracts](https://en.cppreference.com/w/cpp/language/contracts) has the same prolem. therefore, to make function has different symbols in debug / release mode, I use `template<bool ndebug>` to achieve it.
