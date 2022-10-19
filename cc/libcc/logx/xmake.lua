--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("logx")
    set_kind("static")
    add_files("**.cc|**_test.cc|**unused**")
target_end()
