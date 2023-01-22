//
// Created by zaiheshi on 1/20/23.
//
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include "xfly_conf.h"

int main() {
    // TODO: hard-code
    const char* configPath = "/home/zaiheshi/CodeReps/xfly/xfly.conf";
    auto conf = Config::getInstance();
    if (!conf->load(configPath)) {
        printf("load config %s failed.", configPath);
        exit(1);
    }
    std::cout << (*conf);
    return 0;
}
