add_rules("mode.debug", "mode.release")

includes("../xmake/*.lua")
set_languages("c++23")
set_encodings("utf-8")

target("pltxt2htm", function()
    set_kind("binary")
    add_files("main.cc")
    add_includedirs("../include")

    add_ldflags("clangxx::-fuse-ld=lld", {force = true})

    if is_plat("windows") or is_plat("mingw") then
        add_syslinks("ntdll")
    end

    if is_mode("release") then
        set_exceptions("no-cxx")
        add_cxxflags("-fno-rtti", {tools = {"clang", "gcc"}})
        add_cxxflags("-fno-unwind-tables", {tools = {"clang", "gcc"}})
        add_cxxflags("-fno-asynchronous-unwind-tables", {tools = {"clang", "gcc"}})
        add_cxxflags("-fno-ident", {tools = {"clang", "gcc"}})
    elseif is_mode("debug") then
        set_warnings("all")
        set_warnings("extra")
    end

    -- get compiler name
    -- on_config(function (target)
    --     print(target)
    --     local toolchains = target:tool("cxx")
    --     compiler_name = path.basename(toolchains)
    -- end)
end)
