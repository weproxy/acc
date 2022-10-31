--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("liblogx")
    set_kind("static")
    set_basename("logx")
    add_files("**.cc|**_test.cc|**unused**")
target_end()
