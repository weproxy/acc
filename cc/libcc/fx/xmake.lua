--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("libfx")
    set_kind("static")
    set_basename("fx")
    add_files("**.cc|**_test.cc|**unused**")
target_end()
