#include <iostream>

#include "TestBloomFilter.cpp"
#include "TestClock.cpp"
#include "TestDb.cpp"
#include "TestExtendibleHashtable.cpp"
#include "TestLRU.cpp"
#include "TestLSMTree.cpp"
#include "TestMemtable.cpp"
#include "TestSST.cpp"
#include "TestUtils.cpp"

int main() {
    bool allTestPassed = true;
    std::vector<std::pair<TestBase *, std::string>> testClasses = {
        std::make_pair(new TestSST(), "TestSST"),                                  // SST Tests
        std::make_pair(new TestUtils(), "TestUtils"),                              // Utils Tests
        std::make_pair(new TestMemtable(), "TestMemtable"),                        // Memtable Tests
        std::make_pair(new TestDb(), "TestDb"),                                    // Db Tests
        std::make_pair(new TestExtendibleHashtable(), "TestExtendibleHashtable"),  // ExtendibleHashtable Tests
        std::make_pair(new TestLRU(), "TestLRU"),                                  // LRU Tests
        std::make_pair(new TestClock(), "TestClock"),                              // Clock Tests
        std::make_pair(new TestLSMTree(), "TestLSMTree"),                          // LSMTree Tests
        std::make_pair(new TestBloomFilter(), "TestBloomFilter")                   // BloomFilter Tests
    };

    for (auto [testClass, name] : testClasses) {
        std::cout << "Running " << name << ":\n";
        allTestPassed &= testClass->RunTests();
        std::cout << "==============================\n";
    }

    if (allTestPassed) {
        std::cout << "All Tests Passed." << std::endl;
    }
}
