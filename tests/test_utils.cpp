
#include "test_base.h"
#include "utils.h"

class TestUtils : public TestBase {
    static bool TestBinarySearch() {
        std::vector<uint64_t> keys(20);
        for (int i = 0; i < 10; i++) {
            keys[i] = i;
        }
        bool result = true;
        for (int i = 0; i < 10; i++) {
            // Keys are even and also are at even indices.
            result &= Utils::BinarySearch(keys, i) == i;
        }
        return result;
    }

    static bool TestGetBinaryFromInt() {
        bool result = true;
        std::string binary5 = Utils::GetBinaryFromInt(22, 5);
        result &= binary5 == "10110";

        std::string binary3 = Utils::GetBinaryFromInt(22, 3);
        result &= binary3 == "110";

        std::string binary7 = Utils::GetBinaryFromInt(22, 7);
        result &= binary7 == "0010110";

        return result;
    }

    static bool TestGetFilenameWithExt() {
        std::string filename = "test";
        std::string result = Utils::GetFilenameWithExt(filename);
        return filename + ".sst" == result;
    }

    static bool TestEnsureDirSlash() {
        bool result = true;
        result &= Utils::EnsureDirSlash("test/") == "test/";
        result &= Utils::EnsureDirSlash("test") == "test/";
        return result;
    }

   public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestBinarySearch, "TestUtils::TestBinarySearch");
        allTestPassed &= assertTrue(TestGetBinaryFromInt, "TestUtils::TestGetBinaryFromInt");
        allTestPassed &= assertTrue(TestGetFilenameWithExt, "TestUtils::TestGetFilenameWithExt");
        allTestPassed &= assertTrue(TestEnsureDirSlash, "TestUtils::TestEnsureDirSlash");
        return allTestPassed;
    }
};
