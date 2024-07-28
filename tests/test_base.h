
#ifndef CSC443_PROJECT_TESTBASE_H
#define CSC443_PROJECT_TESTBASE_H

#include <functional>
#include <iostream>
#include <string>

class TestBase {
   protected:
    static bool assertTrue(const std::function<bool()> &test, const std::string &name) {
        std::cout << "  - " << name << " - ";
        if (!(std::invoke(test))) {
            std::cout << "FAILED " << "\xE2\x9D\x8C" << std::endl;
            return false;
        }
        std::cout << "passed " << "\xE2\x9C\x94" << std::endl;
        return true;
    }

   public:
    TestBase() = default;

    virtual bool RunTests() = 0;
};

#endif  // CSC443_PROJECT_TESTBASE_H
