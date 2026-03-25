#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <tuple>
#include <array>

#include "CsvSetParser.h" 

TEST(CsvSetParserTest, ParsesValidData) 
{
    std::string csv_data =
    {
        "id;score;ratio\n"
        "1;100;1.5\n"
        "2;200;2.5\n"
    };

    std::istringstream csv_stream(
        csv_data
    );

    CsvSetParser<int, int, double> parser({ "id", "score", "ratio" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    ASSERT_NE(opt, std::nullopt);

    auto result = *opt;
    ASSERT_EQ(result.size(), 2); 

    ASSERT_EQ(std::get<0>(result[0]), 1);
    ASSERT_EQ(std::get<1>(result[0]), 100);
    ASSERT_DOUBLE_EQ(std::get<2>(result[0]), 1.5);

    ASSERT_EQ(std::get<0>(result[1]), 2);
    ASSERT_EQ(std::get<1>(result[1]), 200);
    ASSERT_DOUBLE_EQ(std::get<2>(result[1]), 2.5);
}

TEST(CsvSetParserTest, HandlesShuffledColumns)
{
    std::string csv_data =
    {
        "ratio;id;score\n" 
        "3.14;42;999\n"
    };

    std::istringstream csv_stream(
        csv_data
    );

    CsvSetParser<int, int, double> parser({ "id", "score", "ratio" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    ASSERT_NE(opt, std::nullopt);

    auto result = *opt;

    ASSERT_EQ(result.size(), 1);

    ASSERT_EQ(std::get<0>(result[0]), 42);
    ASSERT_EQ(std::get<1>(result[0]), 999);
    ASSERT_DOUBLE_EQ(std::get<2>(result[0]), 3.14);
}


TEST(CsvSetParserTest, IgnoresExtraColumns) 
{
    std::string csv_data =
    {
        "id;trash_data;score;more_trash\n"
        "5;blablabla;50;useless\n"
    };

    std::istringstream csv_stream(
        csv_data
    );

    CsvSetParser<int, int> parser({ "id", "score" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    auto result = *opt;

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(std::get<0>(result[0]), 5);
    ASSERT_EQ(std::get<1>(result[0]), 50);
}


TEST(CsvSetParserTest, RejectsMissingRequiredColumns) 
{
    std::string csv_data =
    {
        "id;wrong_name\n"
        "1;100\n"
    };
    
    std::istringstream csv_stream(
        csv_data
    );

    CsvSetParser<int, int> parser({ "id", "score" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    ASSERT_FALSE(opt);
}

TEST(CsvSetParserTest, RejectsFileWithInvalidDataTypes) 
{
    std::string csv_data =
    {
        "id;value\n"
        "1;10\n"
        "TWO;20\n" 
        "3;30\n"
    };

    std::istringstream csv_stream(
        csv_data
    );

    CsvSetParser<int, int> parser({ "id", "value" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    ASSERT_FALSE(opt);
}

TEST(CsvSetParserTest, HandlesEmptyStream) 
{
    std::string csv_data = "";
    std::istringstream empty_stream(csv_data);

    CsvSetParser<int> parser({ "id" });
    auto opt = parser.parseSingleStream(empty_stream, csv_data.size());

    ASSERT_FALSE(opt);
}

TEST(CsvSetParserTest, HandlesHeadersOnly) 
{
    std::string csv_data = "id;value";

    std::istringstream csv_stream(csv_data); 

    CsvSetParser<int, int> parser({ "id", "value" });
    auto opt = parser.parseSingleStream(csv_stream, csv_data.size());

    ASSERT_TRUE(opt);

    auto result = *opt;

    ASSERT_TRUE(result.empty());
}
