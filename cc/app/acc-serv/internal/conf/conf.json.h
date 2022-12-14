//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

const char* DEFAULT_CONF = R"({
    "iface": {
        "in": "",
        "out": "jp1"
    },
    "server": [
        {
            "proto": "dns",
            "listen": ":15353"
        },
        {
            "proto": "s5",
            "listen": ":19780",
            "encrypt":true,
            "auth": [
                "USER831173:PASS331250181986"
            ]
        },
        {
            "proto": "ss",
            "listen": ":19781",
            "auth": "aes-256-cfb:123456aa!!"
        },
        {
            "proto": "qc",
            "disabled": true,
            "listen": ":19782",
            "encrypt":true
        },
        {
            "proto": "kc",
            "disabled": true,
            "listen": ":19783",
            "encrypt":true
        },
        {
            "proto": "htp",
            "listen": ":19784"
        }
    ]
})";
