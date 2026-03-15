#include <gtest/gtest.h>

#include "Config.h"

TEST(TomlConfigProcessing, DefaultParamsTest)
{
	std::string config = R"(
	[main]
	input = 'input_dir'
	output = 'output_dir'
	filename_mask = ['mask1', 'mask2']
	)";

	auto res = processTomlString(config);
	
	ASSERT_NE(res, std::nullopt);

	ASSERT_EQ(res->m_input, "input_dir");
	ASSERT_EQ(res->m_output, "output_dir");
	ASSERT_EQ(res->m_filename_masks[0], "mask1");
	ASSERT_EQ(res->m_filename_masks[1], "mask2");
}

TEST(TomlConfigProcessing, OnlyRequiredParams)
{
	std::string config = R"(
	[main]
	input = 'input_dir'
	)";

	auto res = processTomlString(config);

	ASSERT_NE(res, std::nullopt);

	ASSERT_EQ(res->m_input, "input_dir");
	ASSERT_EQ(res->m_output, "output");
	ASSERT_EQ(res->m_filename_masks.size(), 0);
}

TEST(TomlConfigProcessing, WrongInputParamTest)
{
	std::string config = R"(
	[main]
	input = 3
	)";

	auto res = processTomlString(config);

	ASSERT_EQ(res, std::nullopt);
}

TEST(TomlConfigProcessing, WrongOutputParamTest)
{
	std::string config = R"(
	[main]
	input = 'input_dir'
	output = 3
	)";

	auto res = processTomlString(config);

	ASSERT_NE(res, std::nullopt);

	ASSERT_EQ(res->m_input, "input_dir");
	ASSERT_EQ(res->m_output, "output");
}

TEST(TomlConfigProcessing, WrongFilenameMaskParamTest1)
{
	std::string config = R"(
	[main]
	input = 'input_dir'
	filename_mask = 3
	)";

	auto res = processTomlString(config);

	ASSERT_NE(res, std::nullopt);

	ASSERT_EQ(res->m_input, "input_dir");
	ASSERT_EQ(res->m_filename_masks.size(), 0);
}

TEST(TomlConfigProcessing, WrongFilenameMaskParamTest2)
{
	std::string config = R"(
	[main]
	input = 'input_dir'
	filename_mask = 'mask1'
	)";

	auto res = processTomlString(config);

	ASSERT_NE(res, std::nullopt);

	ASSERT_EQ(res->m_input, "input_dir");
	ASSERT_EQ(res->m_filename_masks[0], "mask1");
}