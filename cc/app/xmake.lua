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

-- target
target(NAME)
    set_kind("binary")

    -- add_includedirs(APPDIR.."/internal")
    add_includedirs(APPDIR)

    add_files(CMDDIR.."/**.cc|**_test.cc|**unused**")
    add_files(SRCDIR.."/**.cc|**_test.cc|**unused**")

    add_deps("biz", "fx", "gx", "nx", "logx", "co")
target_end()
