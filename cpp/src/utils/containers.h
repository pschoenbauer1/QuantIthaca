#pragma

namespace utils {
template <typename Container>
int size(const Container& container) {
    return static_cast<int>(container.size());
}
}  // namespace utils