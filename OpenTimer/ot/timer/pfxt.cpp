#include <ot/timer/timer.hpp>

// #define _DEBUG

namespace ot
{

	// Constructor
	PfxtNode::PfxtNode(float s, size_t f, size_t t, const Arc *a, const PfxtNode *p, size_t p_i) : slack{s},
																								   from{f},
																								   to{t},
																								   arc{a},
																								   parent{p},
																								   parent_idx{p_i} {}

	// ------------------------------------------------------------------------------------------------

	// Constructor
	PfxtCache::PfxtCache(const SfxtCache &sfxt) : _sfxt{sfxt}
	{
	}

	// Move constructor
	PfxtCache::PfxtCache(PfxtCache &&pfxt) : _sfxt{pfxt._sfxt},
											 _comp{pfxt._comp},
											 _paths{std::move(pfxt._paths)},
											 _nodes{std::move(pfxt._nodes)}
	{
	}

	// Procedure: _push
	void PfxtCache::_push(float s, size_t f, size_t t, const Arc *a, const PfxtNode *p, size_t p_i)
	{
		_nodes.emplace_back(std::make_unique<PfxtNode>(s, f, t, a, p, p_i));
		std::push_heap(_nodes.begin(), _nodes.end(), _comp);
	}

	// Procedure: _pop
	// Pop a path from the min-heap to the path vector. Here we need to keep the pointer
	// ownership since the later path peeling process need access to the prefix tree node.
	PfxtNode *PfxtCache::_pop()
	{
		if (_nodes.empty())
		{
			return nullptr;
		}
		// 从最小堆中弹出一条路径，并将其添加到路径向量中
		std::pop_heap(_nodes.begin(), _nodes.end(), _comp);
		_paths.push_back(std::move(_nodes.back()));
		_nodes.pop_back();
		return _paths.back().get();
	}

	// Function: _top
	PfxtNode *PfxtCache::_top() const
	{
		return _nodes.empty() ? nullptr : _nodes.front().get();
	}

	// ------------------------------------------------------------------------------------------------

	// Function: _pfxt_cache
	// Construct a prefix tree from a given suffix tree.
	PfxtCache Timer::_pfxt_cache(const SfxtCache &sfxt) const
	{

		PfxtCache pfxt(sfxt);

		// slack还是rat
		std::cout << "_pfxt_cache: " << *sfxt.slack() << std::endl;
		assert(sfxt.slack());

		// Generate the path prefix from each startpoint.
		for (const auto &[k, v] : sfxt._srcs)
		{

			auto [pin, vrf] = _decode_pin(k);

			if (!v)
			{
				continue;
			}
			// MAX: rat - delay - at || s = slack = rat - at
			// MIN:-rat + delay + at || s = slack = -rat + at
			else if (auto s = *sfxt.__dist[k] + *v; s < 1000.0f)
			{

				std::cout << "datasource: " << pin->name() << ": " << s << std::endl;
				// 虚拟边没有对应的Arc，也没有parent pfxtnode
				pfxt._push(s, sfxt._S, k, nullptr, nullptr, 0);
			}
		}
		std::cout << std::endl;
		return pfxt;
	}

	// Function: _my_pfxt_cache
	// Construct a prefix tree from a given suffix tree.
	PfxtCache Timer::_my_pfxt_cache(const SfxtCache &sfxt) const
	{

		PfxtCache pfxt(sfxt);

		assert(sfxt.__dist[sfxt._T]);

		// rat
#ifdef _DEBUG
		std::cout << "_my_pfxt_cache: " << *sfxt.__dist[sfxt._T] << std::endl;
#endif
		// 创建第一个偏离边root
		auto [upin, urf] = _decode_pin(sfxt._T);
		auto el = sfxt._el;
		for (auto arc : upin->_fanin)
		{
			FOR_EACH_RF_IF(vrf, arc->_delay[el][vrf][urf])
			{
				auto v = _encode_pin(arc->_from, vrf);
				auto w = (el == MIN) ? *arc->_delay[el][vrf][urf] : -(*arc->_delay[el][vrf][urf]);
				auto s = *sfxt.__dist[v] + w;
#ifdef _DEBUG
				std::cout << "from: " << arc->_from.name() << " to: "
						  << arc->_to.name() << " slack: " << s
						  << std::endl;
#endif
				if (s < 1000.0f)
				{
					pfxt._push(s, sfxt._T, v, arc, nullptr, 0);
				}
			}
		}
		return pfxt;
	}

	// Procedure: _spur
	// Spur the path and expands the search space. The procedure iteratively scan the present
	// critical path and performs spur operation along the path to generate other candidates.
	void Timer::_spur(Endpoint &ept, size_t K, PathHeap &heap) const
	{
		// std::cout << "sfxt: " << std::endl;
		auto sfxt = _sfxt_cache(ept);
		// 构建虚拟边对应的偏离边，同时按照slack大小顺序排列
		// std::cout << "pfxt: " << std::endl;
		auto pfxt = _pfxt_cache(sfxt);

		for (size_t k = 0; k < K; ++k)
		{

			// 从nodes中弹出一条路径，并将其添加到paths向量中，循环K次后获得了K条最差路径
			auto node = pfxt._pop();

			// no more path to generate
			if (node == nullptr)
			{
				break;
			}

			// If the maximum among the minimum is smaller than the current minimum,
			// there is no need to do more.
			if (heap.num_paths() >= K && heap.top()->slack <= node->slack)
			{
				break;
			}

			// push the path to the heap and maintain the top-k
			auto path = std::make_unique<Path>(node->slack, &ept);
			_recover_datapath(*path, sfxt, node, sfxt._T);

			heap.push(std::move(path));
			heap.fit(K);

			// expand the search space
			_spur(pfxt, *node);
		}
	}

	// Procedure: _spur
	void Timer::_spur(PfxtCache &pfxt, const PfxtNode &pfx) const
	{

		auto el = pfxt._sfxt._el;
		// 首先从虚拟边开始，沿着路径向root扩展
		auto u = pfx.to;

		// while u != root
		while (u != pfxt._sfxt._T)
		{
			assert(pfxt._sfxt.__link[u]);
			// u是DFF的CK
			auto [upin, urf] = _decode_pin(u);

			// for each fan_out of u
			for (auto arc : upin->_fanout)
			{

				FOR_EACH_RF_IF(vrf, arc->_delay[el][urf][vrf])
				{
					// v = head[e]
					auto v = _encode_pin(arc->_to, vrf);

					// skip if the edge goes outside the sfxt
					if (!pfxt._sfxt.__dist[v]) // unreachable
					{
						continue;
					}

					// skip if the edge belongs to the suffix tree
					if (_encode_arc(*arc, urf, vrf) == *pfxt._sfxt.__link[u])
					{
						continue;
					}

					auto w = (el == MIN) ? *arc->_delay[el][urf][vrf] : -(*arc->_delay[el][urf][vrf]);
					auto s = *pfxt._sfxt.__dist[v] + w - *pfxt._sfxt.__dist[u] + pfx.slack;

					// 和违例路径的slack值有关
					if (s < 1000.0f)
					{
						pfxt._push(s, u, v, arc, &pfx, 0);
					}
				}
			}
			u = *pfxt._sfxt.__tree[u];
		}
	}

	void Timer::_my_spur(Endpoint &ept, size_t K, PathHeap &heap) const
	{

		auto sfxt = _my_sfxt_cache(ept);
		// 构建虚拟边对应的偏离边，同时按照slack大小顺序排列
		auto pfxt = _my_pfxt_cache(sfxt);

		for (size_t k = 0; k < K; ++k)
		{

			// 从nodes中弹出一条路径，并将其添加到paths向量中，循环K次后获得了K条最差路径
			auto node = pfxt._pop();

			// no more path to generate
			if (node == nullptr)
			{
				break;
			}

			// If the maximum among the minimum is smaller than the current minimum,
			// there is no need to do more.
			if (heap.num_paths() >= K && heap.top()->slack <= node->slack)
			{
				break;
			}

#ifdef _DEBUG
			auto u = node->from;
			auto [upin, urf] = _decode_pin(u);

			auto v = node->to;
			auto [vpin, vrf] = _decode_pin(v);

			std::cout << "process devition node from: " << upin->name()
					  << " to: " << vpin->name() << std::endl;
#endif
			// push the path to the heap and maintain the top-k
			auto path = std::make_unique<Path>(node->slack, &ept);
			node->path = path.get();

			_my_recover_datapath(*path, sfxt, node, sfxt._S);
#ifdef _DEBUG
			std::cout << "_my_spur: " << node->path << std::endl;
#endif
			heap.push(std::move(path));
			heap.fit(K);

			// expand the search space
			_my_spur(pfxt, *node);
		}
	}

	void Timer::_my_spur(PfxtCache &pfxt, const PfxtNode &pfx) const
	{

		auto el = pfxt._sfxt._el;
		// 首先从虚拟边开始，沿着路径向root扩展
		auto u = pfx.to;

		// while u != root
		size_t i = 0;
		while (pfxt._sfxt.__link[u])
		{
			i++;

			auto [upin, urf] = _decode_pin(u);

			for (auto arc : upin->_fanin)
			{
				FOR_EACH_RF_IF(vrf, arc->_delay[el][vrf][urf])
				{

					// v = head[e]
					auto v = _encode_pin(arc->_from, vrf);

					// skip if the edge goes outside the sfxt
					if (!pfxt._sfxt.__dist[v]) // unreachable
					{
						// 不应该进入这个函数才对
						std::cout << "skip if the edge goes outside the sfxt" << std::endl;
						continue;
					}

					// skip if the edge belongs to the suffix tree
					if (_encode_arc(*arc, vrf, urf) == *pfxt._sfxt.__link[u])
					{
						continue;
					}
#ifdef _DEBUG
					std::cout << "_my_spur from: " << upin->name()
							  << " to: " << arc->_from.name() << std::endl;
#endif
					auto w = (el == MIN) ? *arc->_delay[el][vrf][urf] : -(*arc->_delay[el][vrf][urf]);
					auto s = *pfxt._sfxt.__dist[v] + w - *pfxt._sfxt.__dist[u] + pfx.slack;

					// 和违例路径的slack值有关
					if (s < 1000.0f)
					{
						pfxt._push(s, u, v, arc, &pfx, pfx.parent_idx + i);
					}
				}
			}
			u = *pfxt._sfxt.__tree[u];
		}
	}

}; // end of namespace ot. -----------------------------------------------------------------------
