//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

const char* DEFAULT_CONF = R"({
    "server": [
        {
            "proto": "s5",
            "enabled": true,
            "listen": ":19780",
            "encrypt":true,
            "auth": [
                "USER831173:PASS331250181986"
            ]
        },
        {
            "proto": "ss",
            "enabled": true,
            "listen": ":19781",
            "auth": "aes-256-cfb:123456aa!!"
        },
        {
            "proto": "qc",
            "enabled": false,
            "listen": ":19782",
            "encrypt":true
        },
        {
            "proto": "kc",
            "enabled": false,
            "listen": ":19783",
            "encrypt":true
        },
        {
            "proto": "htp",
            "enabled": true,
            "listen": ":19784"
        }
    ]
})";
