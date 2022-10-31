--
-- weproxy@foxmail.com 2022/10/03
--

-- target
target("libbiz")
    set_kind("static")
    set_basename("biz")
    add_files("**.cc|**_test.cc")
target_end()
