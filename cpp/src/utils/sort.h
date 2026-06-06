#pragma once

#include <glog/logging.h>

#include <algorithm>
#include <vector>

namespace utils {

template <typename T>
bool is_sorted(const std::vector<T>& vec) {
    for (int i = 0; i < static_cast<int>(vec.size()) - 1; ++i) {
        if (vec[i] > vec[i + 1]) {
            return false;
        }
    }
    return true;
}

namespace {
template <typename T>
void quick_sort(std::vector<T>& input, int begin, int end) {
    if (end - begin <= 1) {
        return;
    }
    if (end - begin == 2) {
        if (input[begin] > input[begin + 1]) {
            std::swap(input[begin], input[begin + 1]);
        }
        return;
    }
    const T& pivot = input[begin];
    int pivot_idx = begin;
    for (int i = begin + 1; i < end; i++) {
        if (input[i] < pivot) {
            ++pivot_idx;
            std::swap(input[pivot_idx], input[i]);
        }
    }
    std::swap(input[pivot_idx], input[begin]);

    quick_sort(input, begin, pivot_idx);
    quick_sort(input, pivot_idx + 1, end);
}
}  // namespace

template <typename T>
std::vector<T> quick_sort(std::vector<T> input) {
    quick_sort(input, 0, static_cast<int>(input.size()));

    return input;
}

namespace {
template <typename T>
void merge_sort(std::vector<T>& vec, int begin, int end, std::vector<T>& workspace) {
    if (begin == end || begin + 1 == end) {
        return;
    }
    if (begin + 2 == end) {
        if (vec[begin] > vec[begin + 1]) {
            std::swap(vec[begin], vec[begin + 1]);
        }
        return;
    }
    int mid = (begin + end) / 2;
    merge_sort(vec, begin, mid, workspace);
    merge_sort(vec, mid, end, workspace);

    int ind1 = begin;
    int ind2 = mid;

    for (int i = begin; i < end; ++i) {
        if (ind2 >= end || (ind1 < mid && vec[ind1] <= vec[ind2])) {
            workspace[i] = vec[ind1++];
        } else {
            workspace[i] = vec[ind2++];
        }
    }

    std::copy(&workspace[begin], &workspace[end], &vec[begin]);
}
}  // namespace

template <typename T>
void merge_sort(std::vector<T>& vec) {
    std::vector<T> workspace(vec.size());
    merge_sort(vec, 0, static_cast<int>(vec.size()), workspace);
}

inline void test_log() {
    LOG(INFO) << "test";
}
}  // namespace utils
