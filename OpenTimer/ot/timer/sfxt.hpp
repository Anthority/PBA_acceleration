#ifndef OT_TIMER_SFXT_HPP_
#define OT_TIMER_SFXT_HPP_

#include <ot/headerdef.hpp>
#include <ot/static/logger.hpp>

namespace ot {

// Class: SfxtCache
// The internal thread-local storage to construct suffix tree for
// path-based timing analysis. The suffix tree is a shortest path tree
// generated from an endpoint.
class SfxtCache {

  friend class Timer;

 public:
  SfxtCache(Split, size_t, size_t);

  SfxtCache(const SfxtCache &) = delete;

  SfxtCache(SfxtCache &&);

  ~SfxtCache();

  SfxtCache &operator=(const SfxtCache &) = delete;

  SfxtCache &operator=(SfxtCache &&) = delete;

  inline std::optional<float> slack() const;

  inline Split split() const;

  inline size_t root() const;

 private:
  Split  _el;
  size_t _S; // super source
  size_t _T; // root

  std::vector<size_t> _pins;

  std::unordered_map<size_t, std::optional<float>> _srcs;

  inline thread_local static std::vector<size_t>                __pins; // 记录所有的pin，没看懂为什么要有两个pins
  inline thread_local static std::vector<std::optional<float>>  __dist; // 记录相对于该树中的root的距离+root.rat
  inline thread_local static std::vector<std::optional<size_t>> __tree; // 记录父节点
  inline thread_local static std::vector<std::optional<size_t>> __link; // 记录连接Arc的编号
  inline thread_local static std::vector<std::optional<bool>>   __spfa; // 有没有被探索过

  inline bool _relax(size_t, size_t, std::optional<size_t>, float);
};

// Function: root
inline size_t SfxtCache::root() const {
	return _T;
}

// Function: slack
inline std::optional<float> SfxtCache::slack() const {
	return __dist[_S];
}

// Function: split
inline Split SfxtCache::split() const {
	return _el;
}

// Function: _relax
inline bool SfxtCache::_relax(size_t u, size_t v, std::optional<size_t> e, float d) {
	// 首先检查 __dist[u] 是否存在，如果不存在，或者 __dist[v] + d 的值小于 __dist[u] 的值
	if (!__dist[u] || *__dist[v] + d < *__dist[u]) {
		__dist[u] = *__dist[v] + d;
		__tree[u] = v;
		__link[u] = e;
		return true;
	}
	return false;
}

}; // end of namespace ot. -----------------------------------------------------------------------

#endif
