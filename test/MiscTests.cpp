#include <gtest/gtest.h>

#include "Misc.h"

TEST(MiscFunctions, validateCsvPath_BasicMatches)
{
    std::vector<std::string> masks = { "dota", "abc", "12" };

    ASSERT_TRUE(validateCsvPath(masks, "dota_file.csv"));

    ASSERT_TRUE(validateCsvPath(masks, "dotabc_file.csv"));

    ASSERT_TRUE(validateCsvPath(masks, "file_abc_test.csv"));

    ASSERT_TRUE(validateCsvPath(masks, "file12.csv"));
}