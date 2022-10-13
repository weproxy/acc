--
-- weproxy@foxmail.com 2022/10/03
--

local NAME = APP
local APPDIR = "./"..APP

-- APPDIR
if APP ~= CMD then
	NAME = APP.."-"..CMD
    if os.exists("./"..NAME) then
        APPDIR = "./"..NAME
    end
end

-- CMDDIR
local CMDDIR = "./"..NAME
if not os.exists(CMDDIR) then
    CMDDIR = "./"..APP.."/cmd/"..CMD
    if not os.exists(CMDDIR) then
        CMDDIR = "./"..APP.."/cmd"
    end
end

-- SRCDIR
local SRCDIR = APPDIR.."/internal"

-- include dir
-- add_includedirs(APPDIR.."/internal")
add_includedirs(APPDIR)

-- target
target(NAME)
    set_kind("binary")

    add_files(CMDDIR.."/**.cc|**_test.cc")
    add_files(SRCDIR.."/**.cc|**_test.cc")

    add_deps("biz", "fx", "gx", "nx", "logx", "co")
target_end()
