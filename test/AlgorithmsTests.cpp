#include <gtest/gtest.h>

#include "Algorithms.h"

// helper comparator
auto int_pred = [](const auto& a, const auto& b)
    {
        return std::get<0>(a) < std::get<0>(b);
    };

// helper for easier creation
data_t<int> make_data(std::initializer_list<int> vals)
{
    data_t<int> d;
    for (auto v : vals)
    {
        d.emplace_back(v);
    }
    return d;
}

TEST(StableSortDataArray, BasicSortEachArray)
{
    data_array_t<int> arr = {
        make_data({3, 1, 2}),
        make_data({6, 5, 4})
    };

    stableSortDataArray(arr, int_pred);

    EXPECT_EQ(arr[0], make_data({ 1,2,3 }));
    EXPECT_EQ(arr[1], make_data({ 4,5,6 }));
}

TEST(StableSortDataArray, RemoveEmptyArrays)
{
    data_array_t<int> arr = {
        make_data({1,2}),
        data_t<int>{},
        data_t<int>{}
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0], make_data({ 1,2 }));
}

TEST(StableSortDataArray, MergeOverlappingRanges)
{
    // ranges overlap: must merge
    data_array_t<int> arr = {
        make_data({1, 5}),
        make_data({3, 7})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0], make_data({ 1,3,5,7 }));
}

TEST(StableSortDataArray, NoMergeNonOverlapping)
{
    // strictly ordered, should NOT merge
    data_array_t<int> arr = {
        make_data({1,2}),
        make_data({3,4})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0], make_data({ 1,2 }));
    EXPECT_EQ(arr[1], make_data({ 3,4 }));
}

TEST(StableSortDataArray, ComplexMergeChain)
{
    // all intersect transitively -> one big merge
    data_array_t<int> arr = {
        make_data({1, 10}),
        make_data({2, 9}),
        make_data({3, 8})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0], make_data({ 1,2,3,8,9,10 }));
}

TEST(StableSortDataArray, PartialMerge)
{
    // first two overlap, third is separate
    data_array_t<int> arr = {
        make_data({1,5}),
        make_data({2,6}),
        make_data({10,20})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0], make_data({ 1,2,5,6 }));
    EXPECT_EQ(arr[1], make_data({ 10,20 }));
}

TEST(StableSortDataArray, StabilityCheck)
{
    // check stable behavior
    data_t<int> d;
    d.emplace_back(1);
    d.emplace_back(1);
    d.emplace_back(1);

    data_array_t<int> arr = { d };

    stableSortDataArray(arr, int_pred);

    // order should remain identical
    EXPECT_EQ(arr[0], d);
}

TEST(StableSortDataArray, AllEmpty)
{
    data_array_t<int> arr = {
        data_t<int>{},
        data_t<int>{}
    };

    stableSortDataArray(arr, int_pred);

    EXPECT_TRUE(arr.empty());
}

TEST(StableSortDataArray, SingleArray)
{
    data_array_t<int> arr = {
        make_data({5,3,1,2,4})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0], make_data({ 1,2,3,4,5 }));
}

TEST(StableSortDataArray, LargeMergeScenario)
{
    data_array_t<int> arr = {
        make_data({1,100}),
        make_data({50,150}),
        make_data({120,200}),
        make_data({300,400})
    };

    stableSortDataArray(arr, int_pred);

    ASSERT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0], make_data({ 1,50,100,120,150,200 }));
    EXPECT_EQ(arr[1], make_data({ 300,400 }));
}