add_rules("mode.debug", "mode.release")

set_languages("c++23")
set_encodings("utf-8")

-- only uses emcc to build wasm
target("pltxt2htm", function()
    set_plat("wasm")
    set_kind("binary")
    add_files("pltxt2htm.cc")
    add_cxxflags("-fno-exceptions")
    add_cxxflags("-fno-rtti")
    add_cxxflags("-fno-unwind-tables")
    add_cxxflags("-fno-asynchronous-unwind-tables")
    if is_mode("release") then
        add_cxflags("-fno-ident")
    end
    add_includedirs("../include")
    add_ldflags("-fuse-ld=lld", {force = true})
    add_ldflags("-flto=thin", {force = true})
    add_ldflags("-s EXPORTED_FUNCTIONS=['_pltxt2html']", {force = true})
    add_ldflags("-s EXPORTED_RUNTIME_METHODS=['ccall']", {force = true})
    set_toolchains("emcc@emscripten")
end
