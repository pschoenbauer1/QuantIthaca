#include <algorithm>
#include <iostream>
#include <vector>

template <typename T>
void merge_sort_impl(std::vector<T>& vec, int begin, int end, std::vector<T>& workspace)
{
    if (begin == end || begin + 1 == end)
    {
        return;
    }
    if (begin + 2 == end)
    {
        if (vec[begin] > vec[begin + 1])
        {
            std::swap(vec[begin], vec[begin + 1]);
        }
        return;
    }
    int mid = (begin + end) / 2;
    merge_sort_impl(vec, begin, mid, workspace);
    merge_sort_impl(vec, mid, end, workspace);

    int ind1 = begin;
    int ind2 = mid;
    int i = begin;
    while (ind1 < mid && ind2 < end)
    {
        if (vec[ind1] <= vec[ind2])
        {
            workspace[i++] = vec[ind1++];
        }
        else
        {
            workspace[i++] = vec[ind2++];
        }
    }
    while (ind1 < mid)
    {
        workspace[i++] = vec[ind1++];
    }
    while (ind2 < end)
    {
        workspace[i++] = vec[ind2++];
    }
    std::copy(&workspace[begin], &workspace[end], &vec[begin]);
}

template <typename T>
void merge_sort(std::vector<T>& vec)
{
    std::vector<T> workspace(vec.size());
    merge_sort_impl(vec, 0, static_cast<int>(vec.size()), workspace);
}

int main()
{
    std::cout << "start\n";
    std::vector<int> v{2, 0, 1};
    std::cout << "before sort\n";
    merge_sort(v);
    for (int x : v)
    {
        std::cout << x << ' ';
    }
    std::cout << '\n';
    return 0;
}
