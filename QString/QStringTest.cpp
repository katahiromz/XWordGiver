#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <chrono>
#include "QString.h"

const int ITERATIONS = 100000000;

template <typename T_FN>
void speed_test(const char *name, T_FN fn)
{
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << name << " time: " << duration << " ms" << std::endl;
}

void speed_test_string(void)
{
    std::string str;

    speed_test("std::string concatnate", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str += "test";
        }
    });

    str.clear();
    speed_test("std::string concatnate 2", [&]() {
        std::string pattern = "test";
        for (int i = 0; i < ITERATIONS; ++i) {
            str += pattern;
        }
    });

    speed_test("std::string find 1", [&]() {
        size_t index = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find('t');
        }
        printf("index: %d\n", (int)index);
    });

    speed_test("std::string find 2", [&]() {
        size_t index = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find("te");
        }
        printf("index: %d\n", (int)index);
    });

    speed_test("std::string find_first_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_first_of("wor");
        }
    });

    speed_test("std::string find_first_not_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_first_not_of("Hello");
        }
    });

    speed_test("std::string find_last_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_last_of("wor");
        }
    });

    speed_test("std::string find_last_not_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_last_not_of("orld!");
        }
    });

    speed_test("std::string copy", [&]() {
        std::string str0 = "SKJDFLKJSLKDFJLSKJDFKSKDLJFKSJDFLKJSDLKFJSLSJDFKLSJDLKFJLSDJFLKSDJLSKJDFLKJSLKDFJLSKJDFKSKDLJFKSJDFLKJSDLKFJSLSJDFKLSJDLKFJLSDJFLKSDJL";
        for (int i = 0; i < ITERATIONS; ++i) {
            str = str0;
        }
    });

    speed_test("std::string resize", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str.resize(1024);
            str.clear();
        }
    });

    speed_test("std::string resize 2", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str.resize(1024, ' ');
            str.clear();
        }
    });

#if 0
    speed_test("std::string string literal", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            std::string str("asdflkajsldflajsdkfjalskdfajlkslalkaksjldfkjalsjdfkajslfjdaslkfdjasldf");
            str += "asdflkajsldflajsdkfjalskdfajlkslalkaksjldfkjalsjdfkajslfjdaslkfdjasldf";
        }
    });
#endif
}

void speed_test_QStringA(void)
{
    QStringA str;

    speed_test("QStringA concatnate", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str += "test";
        }
    });

    str.clear();
    speed_test("QStringA concatnate 2", [&]() {
        QStringA pattern = "test";
        for (int i = 0; i < ITERATIONS; ++i) {
            str += pattern;
        }
    });

    speed_test("QStringA find 1", [&]() {
        size_t index = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find('t');
        }
        printf("index: %d\n", (int)index);
    });

    speed_test("QStringA find 2", [&]() {
        size_t index = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find("te");
        }
        printf("index: %d\n", (int)index);
    });

    speed_test("QStringA find_first_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_first_of("wor");
        }
    });

    speed_test("QStringA find_first_not_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_first_not_of("Hello");
        }
    });

    speed_test("QStringA find_last_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_last_of("wor");
        }
    });

    speed_test("QStringA find_last_not_of", [&]() {
        size_t index = 0;
        str = "Hello, world!";
        for (int i = 0; i < ITERATIONS; ++i) {
            index += str.find_last_not_of("orld!");
        }
    });

    speed_test("QStringA copy", [&]() {
        QStringA str0 = "SKJDFLKJSLKDFJLSKJDFKSKDLJFKSJDFLKJSDLKFJSLSJDFKLSJDLKFJLSDJFLKSDJLSKJDFLKJSLKDFJLSKJDFKSKDLJFKSJDFLKJSDLKFJSLSJDFKLSJDLKFJLSDJFLKSDJL";
        for (int i = 0; i < ITERATIONS; ++i) {
            str = str0;
        }
    });

    speed_test("QStringA resize", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str.resize(1024);
            str.clear();
        }
    });

    speed_test("QStringA resize 2", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            str.resize(1024, ' ');
            str.clear();
        }
    });

#if 0
    speed_test("QStringA string literal", [&]() {
        for (int i = 0; i < ITERATIONS; ++i) {
            QStringA str(QStringLiteral("asdflkajsldflajsdkfjalskdfajlkslalkaksjldfkjalsjdfkajslfjdaslkfdjasldf"));
            str += QStringLiteral("asdflkajsldflajsdkfjalskdfajlkslalkaksjldfkjalsjdfkajslfjdaslkfdjasldf");
        }
    });
#endif
}

void test_string(void)
{
    std::cout << "test_string" << std::endl;

    std::string str1("Hello");
    assert(str1.size() == 5);
    assert(strcmp(str1.c_str(), "Hello") == 0);

    std::string str2(str1);
    assert(str1 == str2);

    str2.clear();
    assert(str2.empty());

    str2 = "World";
    assert(str2.size() == 5);
    assert(strcmp(str2.c_str(), "World") == 0);

    str2 += '!';
    assert(str2.size() == 6);
    assert(strcmp(str2.c_str(), "World!") == 0);

    str2.insert(5, " Hello");
    assert(strcmp(str2.c_str(), "World Hello!") == 0);

    str2.erase(5, 6);
    assert(strcmp(str2.c_str(), "World!") == 0);

    str2.replace(0, 5, "Goodbye");
    assert(strcmp(str2.c_str(), "Goodbye!") == 0);

    assert(str1.compare("Hello") == 0);
    assert(str1.compare("World") < 0);
    assert(str1.compare("Apple") > 0);

    //assert(str1.icompare("hello") == 0);
    //assert(str1.icompare("WORLD") < 0);
    //assert(str1.icompare("apple") > 0);

    assert(str1.find('e') == 1);
    assert(str1.find("lo") == 3);
    assert(str1.rfind('l') == 3);
    assert(str1.rfind("el") == 1);
    //assert(str1.ifind('E') == 1);
    //assert(str1.ifind("LO") == 3);
    assert(str1.find("llo") == 2);
    assert(str1.find("LLO") == str1.npos);
    //assert(str1.ifind("LLO") == 2);
    assert(str1.rfind("l") == 3);

    std::string sub = str1.substr(1, 3);
    assert(strcmp(sub.c_str(), "ell") == 0);

    //std::string fmt;
    //fmt.format("%s, %d", "Number", 42);
    //assert(strcmp(fmt.c_str(), "Number, 42") == 0);

    assert(std::to_string(0) == "0");
    assert(std::to_string(-1) == "-1");
    assert(std::to_string(100) == "100");
    assert(std::to_string(0x7FFFFFFF) == "2147483647");
    assert(std::to_string(0x80000000UL) == "2147483648");

    std::cout << "hash: " << std::hash<std::string>{}(sub) << std::endl;

    str1 = "Hello, World!";
    str2 = "123";
    assert(str1.find_first_of(str2) == str1.npos);
    assert(str1.find_first_not_of(str2) == 0);
    assert(str1.find_last_of(str2) == str1.npos);
    assert(str1.find_last_not_of(str2) == str1.size() - 1);

    str1 = "Hello, World!";
    str2 = "WOR!";
    assert(str1.find_first_of(str2) == 7);
    assert(str1.find_first_not_of(str2) == 0);
    assert(str1.find_last_of(str2) == 12);
    assert(str1.find_last_not_of(str2) == 11);
    assert(str1.find_first_of(str2, 0) == 7);
    assert(str1.find_first_not_of(str2, 0) == 0);
    assert(str1.find_last_of(str2, 0) == str1.npos);
    assert(str1.find_last_not_of(str2, 0) == 0);
    assert(str1.find_first_of(str2, 13) == str1.npos);
    assert(str1.find_first_not_of(str2, 13) == str1.npos);
    assert(str1.find_last_of(str2, 13) == 12);
    assert(str1.find_last_not_of(str2, 13) == 11);
    assert(str1.find_first_of(str2, str1.npos) == str1.npos);
    assert(str1.find_first_not_of(str2, str1.npos) == str1.npos);
    assert(str1.find_last_of(str2, str1.npos) == 12);
    assert(str1.find_last_not_of(str2, str1.npos) == 11);
    assert(str1.find_first_of("WOR!") == 7);
    assert(str1.find_first_not_of("WOR!") == 0);
    assert(str1.find_last_of("WOR!") == 12);
    assert(str1.find_last_not_of("WOR!") == 11);
    assert(str1.find_first_of("WOR!", 0) == 7);
    assert(str1.find_first_not_of("WOR!", 0) == 0);
    assert(str1.find_last_of("WOR!", 0) == str1.npos);
    assert(str1.find_last_not_of("WOR!", 0) == 0);
    assert(str1.find_first_of("WOR!", 13) == str1.npos);
    assert(str1.find_first_not_of("WOR!", 13) == str1.npos);
    assert(str1.find_last_of("WOR!", 13) == 12);
    assert(str1.find_last_not_of("WOR!", 13) == 11);
    assert(str1.find_first_of("WOR!", str1.npos) == str1.npos);
    assert(str1.find_first_not_of("WOR!", str1.npos) == str1.npos);
    assert(str1.find_last_of("WOR!", str1.npos) == 12);
    assert(str1.find_last_not_of("WOR!", str1.npos) == 11);

    str1 = "TEST";
    str2 = "TESTTEST";
    assert(str1 < str2);

    str1 = "TESTTEST";
    assert(str1 > "TEST");
}

void test_QStringA(void)
{
    std::cout << "test_QStringA" << std::endl;

    QStringA str1("Hello");
    assert(str1.size() == 5);
    assert(strcmp(str1.c_str(), "Hello") == 0);

    QStringA str2(str1);
    assert(str1 == str2);

    str2.clear();
    assert(str2.empty());

    str2 = "World";
    assert(str2.size() == 5);
    assert(strcmp(str2.c_str(), "World") == 0);

    str2 += '!';
    assert(str2.size() == 6);
    assert(strcmp(str2.c_str(), "World!") == 0);

    str2.insert(5, " Hello");
    assert(strcmp(str2.c_str(), "World Hello!") == 0);

    str2.erase(5, 6);
    assert(strcmp(str2.c_str(), "World!") == 0);

    str2.replace(0, 5, "Goodbye");
    assert(strcmp(str2.c_str(), "Goodbye!") == 0);

    assert(str1.compare("Hello") == 0);
    assert(str1.compare("World") < 0);
    assert(str1.compare("Apple") > 0);

    assert(str1.icompare("hello") == 0);
    assert(str1.icompare("WORLD") < 0);
    assert(str1.icompare("apple") > 0);

    assert(str1.find('e') == 1);
    assert(str1.find("lo") == 3);
    assert(str1.rfind('l') == 3);
    assert(str1.rfind("el") == 1);
    assert(str1.ifind('E') == 1);
    assert(str1.ifind("LO") == 3);
    assert(str1.find("llo") == 2);
    assert(str1.find("LLO") == str1.npos);
    assert(str1.ifind("LLO") == 2);
    assert(str1.rfind("l") == 3);

    QStringA sub = str1.substr(1, 3);
    assert(strcmp(sub.c_str(), "ell") == 0);

    QStringA fmt;
    fmt.format("%s, %d", "Number", 42);
    assert(strcmp(fmt.c_str(), "Number, 42") == 0);

    assert(to_QStringA(0) == "0");
    assert(to_QStringA(-1) == "-1");
    assert(to_QStringA(100) == "100");
    assert(to_QStringA(0x7FFFFFFF) == "2147483647");
    assert(to_QStringA(0x80000000UL) == "2147483648");

    std::cout << "hash: " << std::hash<QStringA>{}(sub) << std::endl;

    str1 = "Hello, World!";
    str2 = "123";
    assert(str1.find_first_of(str2) == str1.npos);
    assert(str1.find_first_not_of(str2) == 0);
    assert(str1.find_last_of(str2) == str1.npos);
    assert(str1.find_last_not_of(str2) == str1.size() - 1);

    str1 = "Hello, World!";
    str2 = "WOR!";
    assert(str1.find_first_of(str2) == 7);
    assert(str1.find_first_not_of(str2) == 0);
    assert(str1.find_last_of(str2) == 12);
    assert(str1.find_last_not_of(str2) == 11);
    assert(str1.find_first_of(str2, 0) == 7);
    assert(str1.find_first_not_of(str2, 0) == 0);
    assert(str1.find_last_of(str2, 0) == str1.npos);
    assert(str1.find_last_not_of(str2, 0) == 0);
    assert(str1.find_first_of(str2, 13) == str1.npos);
    assert(str1.find_first_not_of(str2, 13) == str1.npos);
    assert(str1.find_last_of(str2, 13) == 12);
    assert(str1.find_last_not_of(str2, 13) == 11);
    assert(str1.find_first_of(str2, str1.npos) == str1.npos);
    assert(str1.find_first_not_of(str2, str1.npos) == str1.npos);
    assert(str1.find_last_of(str2, str1.npos) == 12);
    assert(str1.find_last_not_of(str2, str1.npos) == 11);
    assert(str1.find_first_of("WOR!") == 7);
    assert(str1.find_first_not_of("WOR!") == 0);
    assert(str1.find_last_of("WOR!") == 12);
    assert(str1.find_last_not_of("WOR!") == 11);
    assert(str1.find_first_of("WOR!", 0) == 7);
    assert(str1.find_first_not_of("WOR!", 0) == 0);
    assert(str1.find_last_of("WOR!", 0) == str1.npos);
    assert(str1.find_last_not_of("WOR!", 0) == 0);
    assert(str1.find_first_of("WOR!", 13) == str1.npos);
    assert(str1.find_first_not_of("WOR!", 13) == str1.npos);
    assert(str1.find_last_of("WOR!", 13) == 12);
    assert(str1.find_last_not_of("WOR!", 13) == 11);
    assert(str1.find_first_of("WOR!", str1.npos) == str1.npos);
    assert(str1.find_first_not_of("WOR!", str1.npos) == str1.npos);
    assert(str1.find_last_of("WOR!", str1.npos) == 12);
    assert(str1.find_last_not_of("WOR!", str1.npos) == 11);

    str1 = "TEST";
    str2 = "TESTTEST";
    assert(str1 < str2);

    str1 = "TESTTEST";
    assert(str1 > "TEST");
}

int main(void)
{
    test_string();
    test_QStringA();

    speed_test_string();
    speed_test_QStringA();

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
