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

TEST(MedianDeviationDataArray, EmptyArrays) {
    data_array_t<double> empty_data = {};
    data_array_t<double> empty_sub_arrays = { {}, {}, {} };

    int call_count = 0;
    auto getter = [](auto v) { return std::get<0>(v); };
    auto callback = [&](auto) { call_count++; };

    medianDeviationDataArray(empty_data, getter, callback);
    medianDeviationDataArray(empty_sub_arrays, getter, callback);

    ASSERT_EQ(call_count, 0);
}

TEST(MedianDeviationDataArray, SingleElement) {
    data_array_t<double> data = { {42.0} };
    std::vector<double> deviated_elements;

    // accuracy = 0.0 to make sure the callback triggers (|42.0 - 0.0| >= 0)
    medianDeviationDataArray(data, [](auto v) { return std::get<0>(v); }, [&](auto v) {
        deviated_elements.push_back(std::get<0>(v));
        }, 0.0);

    ASSERT_EQ(deviated_elements.size(), 1);
    EXPECT_DOUBLE_EQ(deviated_elements[0], 42.0);
}

// Test 3: Check correct incremental median calculation (output all elements)
TEST(MedianDeviationDataArray, AccurateMedianCalculation) {
    data_array_t<double> data = { {0.0, 20.0, 30.0, 40.0, 50.0} };
    std::vector<double> deviated_elements;

    // With accuracy = 0.0, callback should trigger at every step
    medianDeviationDataArray(data, [](auto v) { return std::get<0>(v); }, [&](auto v) {
        deviated_elements.push_back(std::get<0>(v));
        }, 0.0);

    // Since accuracy = 0.0, callback should trigger for every added element
    std::vector<double> expected = { 0.0, 20.0, 30.0, 40.0, 50.0 };
    ASSERT_EQ(deviated_elements, expected);
}

// Test 4: Check working of the accuracy threshold
TEST(MedianDeviationDataArray, AccuracyThresholdFiltering) {
    // Input data: 10, 20, 30, 40, 50
    data_array_t<double> data = { {10.0, 20.0, 30.0, 40.0, 50.0} };
    std::vector<double> deviated_elements;

    // Median steps:
    // 1: (x=10) med = 10. prev = 0.  |10-0| = 10.  10 >= 6 (TRIGGER on 10.0) -> prev becomes 10
    // 2: (x=20) med = 15. prev = 10. |15-10| = 5.  5 < 6   (IGNORE) -> prev becomes 15
    // 3: (x=30) med = 20. prev = 15. |20-15| = 5.  5 < 6   (IGNORE) -> prev becomes 20
    // 4: (x=40) med = 25. prev = 20. |25-20| = 5.  5 < 6   (IGNORE) -> prev becomes 25
    // 5: (x=50) med = 30. prev = 25. |30-25| = 5.  5 < 6   (IGNORE) -> prev becomes 30

    medianDeviationDataArray(data, [](auto v) { return std::get<0>(v); }, [&](auto v) {
        deviated_elements.push_back(std::get<0>(v));
        }, 6.0); // accuracy = 6.0

    ASSERT_EQ(deviated_elements.size(), 1);
    EXPECT_DOUBLE_EQ(deviated_elements[0], 10.0);
}

// Test 5: Work with chunked 2D arrays
TEST(MedianDeviationDataArray, Chunked2DArrays) {
    // Same array, but split into nested vectors differently,
    // including empty inner arrays.
    data_array_t<double> chunked_data = { {0.0, 20.0}, {}, {30.0}, {40.0, 50.0} };
    std::vector<double> deviated_elements;

    medianDeviationDataArray(chunked_data, [](auto v) { return std::get<0>(v); }, [&](auto v) {
        deviated_elements.push_back(std::get<0>(v));
        }, 0.0);

    // Result should be identical to the whole array
    std::vector<double> expected = { 0.0, 20.0, 30.0, 40.0, 50.0 };
    ASSERT_EQ(deviated_elements, expected);
}