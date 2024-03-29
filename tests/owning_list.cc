#include "test.h"
#include <panda/owning_list.h>

TEST_PREFIX("owning_list: ", "[owning_list]");

TEST("empty") {
    owning_list<int> list;
    REQUIRE(list.size() == 0);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        FAIL("list must be empty");
    }
    REQUIRE(true);
}

TEST("simple") {
    owning_list<int> list;
    list.push_back(10);
    REQUIRE(list.size() == 1);
    auto iter = list.begin();
    for (; iter != list.end(); ++iter) {
        REQUIRE(*iter == 10);
        break;
    }
    REQUIRE(++iter == list.end());
}

TEST("2 elements") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(1);
    REQUIRE(list.size() == 2);
    auto iter = list.begin();
    int i = 0;
    for (; iter != list.end(); ++iter) {
        REQUIRE(*iter == i);
        ++i;
    }
    REQUIRE(iter == list.end());
}

TEST("ctor from initializer list") {
    owning_list<int> list = {1,2,3};
    CHECK(list.size() == 3);
    auto iter = list.begin();
    CHECK(*iter++ == 1);
    CHECK(*iter++ == 2);
    CHECK(*iter++ == 3);
    CHECK(iter == list.end());
}

TEST("assign initializer list") {
    owning_list<int> list = {1,2,3};
    list = {3,2};
    CHECK(list.size() == 2);
    auto iter = list.begin();
    CHECK(*iter++ == 3);
    CHECK(*iter++ == 2);
    CHECK(iter == list.end());
}

TEST("copy ctor") {
    owning_list<int> list = {1,2,3};
    owning_list<int> copy = list;

    auto iter = list.begin();
    CHECK(*iter++ == 1);
    CHECK(*iter++ == 2);
    CHECK(*iter++ == 3);
    CHECK(iter == list.end());

    iter = copy.begin();
    CHECK(*iter++ == 1);
    CHECK(*iter++ == 2);
    CHECK(*iter++ == 3);
    CHECK(iter == copy.end());

    list.remove(2);
    CHECK(list.size() == 2);
    CHECK(copy.size() == 3);
}

TEST("copy assign") {
    owning_list<int> list = {1,2,3};
    owning_list<int> copy = {8,9};
    copy = list;

    auto iter = list.begin();
    CHECK(*iter++ == 1);
    CHECK(*iter++ == 2);
    CHECK(*iter++ == 3);
    CHECK(iter == list.end());

    iter = copy.begin();
    CHECK(*iter++ == 1);
    CHECK(*iter++ == 2);
    CHECK(*iter++ == 3);
    CHECK(iter == copy.end());

    list.remove(2);
    CHECK(list.size() == 2);
    CHECK(copy.size() == 3);
}

TEST("remove 0") {
    owning_list<int> list;
    list.push_back(10);
    list.remove(10);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        FAIL("list must be empty");
    }
    REQUIRE(list.size() == 0);
}


TEST("remove 1") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(2);
    list.remove(2);
    list.push_back(1);
    int i = 0;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        REQUIRE(*iter == i++);
    }
    REQUIRE(list.size() == 2);
}

TEST("remove in iteration") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(2);
    list.push_back(1);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        list.remove(2);
    }
    int i = 0;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        REQUIRE(*iter == i++);
    }

    REQUIRE(list.size() == 2);
}

TEST("remove in iteration 2") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(1);
    list.push_back(2);
    auto iter = list.begin();
    list.remove(1);
    list.remove(2);
    ++iter;
    REQUIRE(iter == list.end());
    REQUIRE(list.size() == 1);
}

TEST("remove in iteration reverse") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(1);
    list.push_back(2);
    auto iter = list.rbegin();
    list.remove(1);
    list.remove(2);
    REQUIRE(*iter == 2);
    //++iter is invalid
    REQUIRE(list.size() == 1);
}

TEST("remove in iteration reverse ++") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(1);
    list.push_back(2);
    auto iter = list.rbegin();
    list.remove(2);
    REQUIRE(*iter++ == 2);
    REQUIRE(*iter++ == 1);
    REQUIRE(*iter++ == 0);
    //++iter is invalid
    REQUIRE(list.size() == 2);
}

TEST("erase in iteration reverse ++") {
    owning_list<int> list;
    list.push_back(0);
    list.push_back(1);
    list.push_back(2);
    auto iter = list.rbegin();
    list.erase(iter);
    REQUIRE(*iter++ == 2);
    REQUIRE(*iter++ == 1);
    REQUIRE(*iter++ == 0);
    //++iter is invalid
    REQUIRE(list.size() == 2);
}

TEST("clear Tracer") {
    Tracer::refresh();
    owning_list<Tracer> list;
    list.push_back(Tracer(0));
    list.push_back(Tracer(1));
    list.push_back(Tracer(2));
    list.clear();
    REQUIRE(Tracer::ctor_total() == Tracer::dtor_calls);

    //++iter is invalid
    REQUIRE(list.size() == 0);
}

TEST("remove Tracer") {
    Tracer::refresh();
    owning_list<Tracer> list;
    list.push_back(Tracer(0));
    list.push_back(Tracer(1));
    list.push_back(Tracer(2));
    list.remove(Tracer(0));
    list.remove(Tracer(2));
    REQUIRE(Tracer::ctor_total() == Tracer::dtor_calls + 1);

    //++iter is invalid
    REQUIRE(list.size() == 1);
}
