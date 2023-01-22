//
// Created by zaiheshi on 1/22/23.
//

#include <catch2/catch.hpp>
#include <cstring>
#include "xfly_func.h"

TEST_CASE("test lStrip") {
    char t1[] = "";
    lStrip(t1);
    CHECK(strcmp(t1, "") == 0);

    char t2[] = "\n\t \r  hello";
    lStrip(t2);
    CHECK(strcmp(t2, "hello") == 0);

    char t3[] = "\n\t \r  \t \n  ";
    lStrip(t3);
    CHECK(strcmp(t3, "") == 0);

    char t4[] = "hello";
    lStrip(t4);
    CHECK(strcmp(t4, "hello") == 0);

    char *t5 = nullptr;
    lStrip(t5);
    CHECK(t5 == nullptr);
}

TEST_CASE("test rStrip") {
    char t1[] = "";
    rStrip(t1);
    CHECK(strcmp(t1, "") == 0);

    char t2[] = "hello\n\t \r  ";
    rStrip(t2);
    CHECK(strcmp(t2, "hello") == 0);

    char t3[] = "\n\t \r  \t \n  ";
    rStrip(t3);
    CHECK(strcmp(t3, "") == 0);

    char t4[] = "hello";
    rStrip(t4);
    CHECK(strcmp(t4, "hello") == 0);

    char *t5 = nullptr;
    rStrip(t5);
    CHECK(t5 == nullptr);
}
