
#include <concepts>
#include <vector>

namespace utils {

template <typename T>
int size(const T& t) {
    return static_cast<int>(t.size());
}

template <typename T>
concept LessThanComparable = requires(const T& left, const T& right) { left < right; };

template <LessThanComparable T>
// requires requires(const T& left, const T& right){ left < right; }
class Heap {
    std::vector<T> _heap;

   public:
    Heap() = default;

    void push(const T& t) {
        _heap.push_back(t);

        int idx = utils::size(_heap) - 1;

        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (_heap[parent] < _heap[idx]) {
                std::swap(_heap[idx], _heap[parent]);
                idx = parent;
            } else {
                break;
            }
        }
    }

    const T& top() const {
        // Error handling?
        return _heap.front();
    }

    void pop() {
        std::swap(_heap[0], _heap[utils::size(_heap) - 1]);
        _heap.resize(utils::size(_heap) - 1);

        const int s = utils::size(_heap);

        int idx = 0;
        while (2 * idx + 1 < s) {
            int child1 = 2 * idx + 1;
            int child2 = 2 * idx + 2;

            int swap_idx = -1;

            if (child1 < s && _heap[idx] < _heap[child1]) {
                swap_idx = child1;
            }
            if (child2 < s && _heap[idx] < _heap[child2] && _heap[child1] < _heap[child2]) {
                swap_idx = child2;
            }
            if (swap_idx == -1) {
                break;
            }
            std::swap(_heap[idx], _heap[swap_idx]);
            idx = swap_idx;
        }
    }
};
}  // namespace utils