#include <context/graph.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <math.h>
#include <py/py_bridge.h>
#include <utils/heap.h>
#include <utils/ringbuffer.h>
#include <utils/sort.h>
#include <utils/threads.h>

#include <algorithm>
#include <core.hpp>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

TEST(UtilsTest, TestThreadPool)
{
    utils::ThreadPool p(4);

    int x = 2;

    p.push([&]() { x = 4; });
    auto future = p.push(
        [&]()
        {
            double y = std::exp(.123);
            return y;
        });

    double v = future.get();

    EXPECT_DOUBLE_EQ(v, std::exp(.123));

    p.wait();

    EXPECT_EQ(x, 4);
}

TEST(UtilsTest, TestQuickSort)
{
    std::vector<double> vec{1., 2., 3., 0., 4.};

    const auto vec_sorted = utils::quick_sort(vec);

    EXPECT_TRUE(utils::is_sorted(vec_sorted));

    for (const auto x : vec_sorted)
    {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
}

namespace
{

template <typename T>
void ExpectMergeSortMatchesStdSort(std::vector<T> input)
{
    const std::vector<T> expected = [&]
    {
        auto copy = input;
        std::sort(copy.begin(), copy.end());
        return copy;
    }();
    utils::merge_sort(input);
    EXPECT_EQ(input, expected);
    EXPECT_TRUE(utils::is_sorted(input));
}

}  // namespace

// TEST(UtilsTest, TestMergeSort)
// {
//     // Empty and single-element.
//     {
//         std::vector<int> empty;
//         utils::merge_sort(empty);
//         EXPECT_TRUE(empty.empty());
//         EXPECT_TRUE(utils::is_sorted(empty));
//     }
//     ExpectMergeSortMatchesStdSort(std::vector<int>{0});
//     ExpectMergeSortMatchesStdSort(std::vector<int>{-1000000000});
//     ExpectMergeSortMatchesStdSort(std::vector<double>{3.14159265358979});

//     // Length 2: compare/swap path only (covers order, ties, signs, floating types).
//     ExpectMergeSortMatchesStdSort(std::vector<int>{0, 1});
//     ExpectMergeSortMatchesStdSort(std::vector<int>{1, 0});
//     ExpectMergeSortMatchesStdSort(std::vector<int>{7, 7});
//     ExpectMergeSortMatchesStdSort(std::vector<int>{-2, -5});
//     ExpectMergeSortMatchesStdSort(
//         std::vector<int>{std::numeric_limits<int>::max(), std::numeric_limits<int>::min()});
//     ExpectMergeSortMatchesStdSort(std::vector<double>{-0.0, 0.0});
//     ExpectMergeSortMatchesStdSort(std::vector<double>{1.25, 1.25});
//     ExpectMergeSortMatchesStdSort(std::vector<double>{2.0, 1.0});

//     // All 3! permutations of three distinct values (covers 3-element merge path).
//     {
//         std::vector<int> v{0, 1, 2};
//         int n = 0;
//         do
//         {
//             // LOG(INFO) << v[0] << v[1] << v[2] << std::endl;
//             ExpectMergeSortMatchesStdSort(v);
//             ++n;
//         } while (std::next_permutation(v.begin(), v.end()));
//         EXPECT_EQ(n, 6);
//     }

//     // 100 elements: already sorted, reversed, and shuffled (deterministic seed).
//     {
//         std::vector<int> sorted(100);
//         std::iota(sorted.begin(), sorted.end(), -50);
//         ExpectMergeSortMatchesStdSort(sorted);
//     }
//     {
//         std::vector<int> rev(100);
//         std::iota(rev.rbegin(), rev.rend(), -50);
//         ExpectMergeSortMatchesStdSort(rev);
//     }
//     {
//         std::vector<int> shuffled(100);
//         std::iota(shuffled.begin(), shuffled.end(), 0);
//         std::mt19937 gen(42);
//         std::shuffle(shuffled.begin(), shuffled.end(), gen);
//         ExpectMergeSortMatchesStdSort(shuffled);
//     }
// }

TEST(UtilsTest, TestHeap)
{
    utils::Heap<double> heap;
    heap.push(1.0);
    heap.push(2.0);
    heap.push(0.0);
    heap.push(1.5);
    heap.push(-100.);
    heap.push(0.0);
    heap.push(-100.);

    EXPECT_EQ(heap.top(), 2.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 1.5);
    heap.pop();
    EXPECT_EQ(heap.top(), 1.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 0.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 0.0);
    heap.pop();
    EXPECT_EQ(heap.top(), -100.0);
    heap.pop();
    EXPECT_EQ(heap.top(), -100.0);
}

TEST(TestCallback, TestCallback)
{
    auto callbackstr = py_bridge::call_python<std::string>("test_callback", "hi there");
    EXPECT_EQ(callbackstr, "Hello C++! Python Here! You said hi there.");
}
TEST(TestOperations, TestDivision)
{
    utils::test_log();
    LOG(INFO) << (2 / 5) << ", " << (0 / 5) << ", " << (-2 / 5) << ", " << (-5 / 5) << ", "
              << (-6 / 5) << std::endl;
    EXPECT_TRUE(true);
}

TEST(UtilsTest, TestSpscRingBuffer)
{
    utils::SpscRingBuffer<int> buffer(1024);

    int N = 1000 * 1000;
    std::vector<int> out(N);

    auto lambda_push = [&]()
    {
        for (int i = 0; i < N; ++i)
        {
            while (!buffer.push(i));
        }
    };
    auto lambda_pop = [&]()
    {
        for (int i = 0; i < N; ++i)
        {
            while (!buffer.pop(out[i]));
        }
    };

    const auto start = std::chrono::system_clock::now();

    std::jthread thread_push(lambda_push);
    std::jthread thread_pop(lambda_pop);

    thread_push.join();
    thread_pop.join();

    const auto end = std::chrono::system_clock::now();

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG(INFO) << "TestSpscRingBuffer: " << duration.count() << " us";

    for (int i = 0; i < N; ++i)
    {
        EXPECT_EQ(out[i], i);
    }
}

TEST(UtilsTest, TestRingBuffer)
{
    utils::RingBuffer<int> buffer(1024);

    int N = 1000 * 1000;
    std::vector<int> out(N);

    auto lambda_push = [&]()
    {
        for (int i = 0; i < N; ++i)
        {
            while (!buffer.push(i));
        }
    };
    auto lambda_pop = [&]()
    {
        for (int i = 0; i < N; ++i)
        {
            while (!buffer.pop(out[i]));
        }
    };

    const auto start = std::chrono::system_clock::now();

    std::jthread thread_push(lambda_push);
    std::jthread thread_pop(lambda_pop);

    thread_push.join();
    thread_pop.join();

    const auto end = std::chrono::system_clock::now();

    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG(INFO) << "TestRingBuffer: " << duration.count() << " us";

    for (int i = 0; i < N; ++i)
    {
        EXPECT_EQ(out[i], i);
    }
}
