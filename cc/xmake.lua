--
-- weproxy@foxmail.com 2022/10/03
--

-- plat
set_config("plat", os.host())

-- project
set_project("acc")

-- set xmake minimum version
set_xmakever("2.3.1")

-- set common flags
set_languages("c++11")
set_warnings("error")     -- -Wall

add_rules("mode.debug")
if is_mode("debug") then
    add_defines("DEBUG")
    set_symbols("debug")    -- dbg symbols
end

if is_plat("windows") then
    add_defines("build_tag_windows")
elseif is_plat("linux") then
    add_defines("build_tag_linux")
elseif is_plat("darwin") then
    add_defines("build_tag_darwin")
end

if is_plat("windows") then
    set_optimize("fastest")  -- faster: -O2  fastest: -Ox  none: -O0
    add_cxflags("/EHsc")
    add_ldflags("/SAFESEH:NO")
    if is_mode("debug") then
        set_runtimes("MTd")
    else
        set_runtimes("MT")
    end
elseif is_plat("mingw") then
    add_ldflags("-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lwinpthread -Wl,-Bdynamic", {force = true})
    set_optimize("faster")
else
    set_optimize("faster")   -- faster: -O2  fastest: -O3  none: -O0
    --add_cxflags("-Wno-narrowing", "-Wno-sign-compare", "-Wno-strict-aliasing")
    if is_plat("macosx", "iphoneos") then
        add_cxflags("-fno-pie")
    end
end

APP = os.getenv("app") or "acc"
CMD = os.getenv("cmd") or "cli"

-- include dir
add_includedirs("libcc")
add_includedirs("libcc/3rd/coost/include")
add_includedirs("libcc/3rd/nlohmann_json/single_include")

-- include sub-projects
includes("libcc")
includes("app")
