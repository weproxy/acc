//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

const char* DEFAULT_CONF = R"({
    "server": {
        "auth": {
            "s5": "",
            "ss": ""
        },
        "dns": [],
        "main": [],
        "geo": {
            "us": [],
            "jp": []
        }
    },

    "dns": [
        {
            "host": [],
            "serv": []
        },
        {
            "host": ["*"],
            "serv": ["dns://s5"]
        }
    ],

    "rules": [
        {
            "host": [],
            "serv": []
        },
        {
            "host": ["*"],
            "serv": ["default"]
        }
    ]
})";
