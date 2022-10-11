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
local SRCDIR = APPDIR.."/src"

-- include dir
add_includedirs(APPDIR.."/src")

-- target
target(NAME)
    set_kind("binary")

    add_files(CMDDIR.."/**.cc|**_test.cc")
    add_files(SRCDIR.."/**.cc|**_test.cc")

    add_deps("cc", "co")    -- libcc libcc/3rd/coost
target_end()
