--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("cc")
    set_kind("static")
    add_files("**.cc|**_test.cc")
    -- add_files("**.cxx|**_test.cxx")
    -- add_files("**.cpp|**_test.cpp")
    remove_files("3rd/**")
target_end()
