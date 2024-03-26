#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <unordered_map>

using namespace std;

struct HashFunc
{
    template <typename T, typename U>
    size_t operator()(const std::pair<T, U> &p) const
    {
        return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
    }
};

// 键值比较，哈希碰撞的比较定义，需要直到两个自定义对象是否相等
struct EqualKey
{
    template <typename T, typename U>
    bool operator()(const std::pair<T, U> &p1, const std::pair<T, U> &p2) const
    {
        return p1.first == p2.first && p1.second == p2.second;
    }
};

int main()
{
    std::unordered_map<std::pair<int, int>, int, HashFunc, EqualKey> mmap;

    mmap[{1, 3}] = 2;
    mmap[{2, 3}] = 4;
    mmap[{3, 3}] = 7;
    mmap[{5, 3}] = 6;
    mmap[{1, 1}] = 1;

    int a, b;
    while (std::cin >> a >> b)
    {
        if (mmap.find({a, b}) != mmap.end())
        {
            std::cout << mmap[{a, b}] << "\n";
        }
    }
    std::system("pause");
}