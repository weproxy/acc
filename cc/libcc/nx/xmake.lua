--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("nx")
    set_kind("static")
    add_files("**.cc|**_test.cc")
target_end()