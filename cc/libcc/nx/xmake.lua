--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("libnx")
    set_kind("static")
    set_basename("nx")
    add_files("**.cc|**_test.cc|**unused**")
target_end()
