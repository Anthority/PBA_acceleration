#include <ot/timer/sfxt.hpp>
#include <ot/timer/timer.hpp>

// #define _DEBUG

namespace ot {

// Constructor
SfxtCache::SfxtCache(Split el, size_t S, size_t T) : _el{el},
													 _S{S},
													 _T{T},
													 _pins{std::move(__pins)} {

	resize_to_fit(std::max(S, T) + 1, __tree, __link, __dist, __spfa);

	// debug
	// for(const auto& i : __tree) assert(!i);
	// for(const auto& i : __dist) assert(!i);
	// for(const auto& i : __spfa) assert(!i);
}

// Move constructor
SfxtCache::SfxtCache(SfxtCache &&rhs) : _el{rhs._el},
										_S{rhs._S},
										_T{rhs._T},
										_pins{std::move(rhs._pins)},
										_srcs{std::move(rhs._srcs)} {
}

// Destructor
SfxtCache::~SfxtCache() {

	__dist[_S].reset();
	__tree[_S].reset();
	__link[_S].reset();
	__spfa[_S].reset();

	for (const auto &p : _pins) {
		__dist[p].reset();
		__tree[p].reset();
		__link[p].reset();
		__spfa[p].reset();
	}

	_pins.clear();
	__pins = std::move(_pins);
}

// ----------------------------------------------------------------------------

// Procedure: _topologize DFS
void Timer::_topologize(SfxtCache &sfxt, size_t v) const {

	sfxt.__spfa[v] = true;
	// 获得pin和对应的tran
	auto [pin, vrf] = _decode_pin(v);

	// Stop at the data source
	if (!pin->is_datapath_source()) {
		for (auto arc : pin->_fanin) {
			// std::cout<<'\"'<<arc->_from.name()<<'\"'<<" -> "<<'\"'<<arc->_to.name()<<'\"'<<";"<<std::endl;

			// rise和fall单独对应一个spfx
			// 通过urf 和 vrf确定当前spfx是单一rf的
			FOR_EACH_RF_IF(urf, arc->_delay[sfxt._el][urf][vrf]) {
					auto u = _encode_pin(arc->_from, urf);
					if (!sfxt.__spfa[u]) {
						// 递归调用，拓补排序是倒序，root在最后
						_topologize(sfxt, u);
					}
				}
		}
	}

	sfxt._pins.push_back(v);
}

// Procedure: _spdp
void Timer::_spdp(SfxtCache &sfxt) const {
	// 如果一开始_pins不为空，就报错
	assert(sfxt._pins.empty());
	// 按照拓补顺序为sfxt._pins添加所有pin
	_topologize(sfxt, sfxt._T);
	// std::cout << std::endl;

	assert(!sfxt._pins.empty());

	auto el = sfxt._el;

	// for (unsigned long v : sfxt._pins) {
	// 	auto [pin, vrf] = _decode_pin(v);
	// 	std::cout << pin->name() << std::endl;
	// }
	// std::cout << std::endl;

	// 从后往前遍历
	for (auto itr = sfxt._pins.rbegin(); itr != sfxt._pins.rend(); ++itr) {

		auto v = *itr;
		auto [pin, vrf] = _decode_pin(v);

		// 一开始在建立spfx的时候，root的dist指定为rat
		assert(sfxt.__dist[v]);

		// Stop at the data source
		if (pin->is_datapath_source()) {
			std::cout << pin->name() << std::endl;
			sfxt._srcs.try_emplace(v, std::nullopt);
			continue;
		}

		// Relax on fanin
		for (auto arc : pin->_fanin) {
			// #define FOR_EACH_RF_IF(rf, c) \
			//   for (auto rf : TRAN)        \
			//     if (c)
			// 建图,建立所有的边
			FOR_EACH_RF_IF(urf, arc->_delay[el][urf][vrf]) {
					auto u = _encode_pin(arc->_from, urf);
					auto d = (el == MIN) ? *arc->_delay[el][urf][vrf] : -(*arc->_delay[el][urf][vrf]);
					sfxt._relax(u, v, _encode_arc(*arc, urf, vrf), d);
				}
		}
	}
}

// Procedure: _spfa
// Perform shortest path fast algorithm (SPFA) to build up the suffix tree.
void Timer::_spfa(SfxtCache &sfxt) const {

	auto el = sfxt._el;

	std::queue<size_t> queue;
	queue.push(sfxt._T);
	sfxt.__spfa[sfxt._T] = true;

	while (!queue.empty()) {

		auto v = queue.front();
		queue.pop();
		sfxt.__spfa[v] = false;
		sfxt._pins.push_back(v);

		auto [pin, vrf] = _decode_pin(v);

		// Stop at the data source
		if (pin->is_datapath_source()) {
			sfxt._srcs.try_emplace(v, std::nullopt);
			continue;
		}

		// Relax on fanin
		for (auto arc : pin->_fanin) {
			FOR_EACH_RF_IF(urf, arc->_delay[el][urf][vrf]) {
					auto u = _encode_pin(arc->_from, urf);
					auto d = (el == MIN) ? *arc->_delay[el][urf][vrf] : -(*arc->_delay[el][urf][vrf]);
					if (sfxt._relax(u, v, _encode_arc(*arc, urf, vrf), d)) {
						if (!sfxt.__spfa[u] || *sfxt.__spfa[u] == false) {
							queue.push(u);
							sfxt.__spfa[u] = true;
						}
					}
				}
		}
	}
}

// Function: _sfxt_cache
// Find the suffix tree rooted at the primary output po.
SfxtCache Timer::_sfxt_cache(const PrimaryOutput &po, Split el, Tran rf) const {
	// 如果没有rat就报错
	assert(po._rat[el][rf]);

	// create a cache
	// _idx2pin.size()：pin的数量 * 2，因为每个pin有rise和fall
	auto S = _idx2pin.size() << 1;
	// 获得po的index
	auto T = _encode_pin(po._pin, rf);

	// 以S和T的最大值初始化容器大小
	SfxtCache sfxt(el, S, T);

	// start at the root
	// 如果刚开始root的dist不为空，就报错
	assert(!sfxt.__dist[T]);
	// MAX:相当于所有的节点都增加了一个 rat
	// MIN:相当于所有的节点都增加了一个-rat
	sfxt.__dist[T] = (el == MIN) ? -(*po._rat[el][rf]) : *po._rat[el][rf];

	// shortest path dynamic programming
	_spdp(sfxt);
	// std::cout << std::endl;

	// shortest path fast algorithm
	//_spfa(sfxt);

	// relax sources
	for (auto &[s, v] : sfxt._srcs) {
		// std::cout << sfxt.root() << " " << sfxt._srcs.size() << std::endl;
		// v是at或者-at
		if (v = _sfxt_offset(sfxt, s); v) {
			sfxt._relax(S, s, std::nullopt, *v);
			auto [pin, vrf] = _decode_pin(s);
			std::cout << "po sfxt: " << po._pin.name() << " " << pin->name() << ": " << *v << std::endl;
		}
	}
	std::cout << std::endl;

	return sfxt;
}

// Function: _sfxt_cache
// Find the suffix tree rooted at the test
SfxtCache Timer::_sfxt_cache(const Test &test, Split el, Tran rf) const {

	assert(test._rat[el][rf]);

	// create a cache
	auto S = _idx2pin.size() << 1;
	auto T = _encode_pin(test._arc._to, rf);

	SfxtCache sfxt(el, S, T);

	// Start at the D pin and perform SPFA all the way to the sources of data paths.
	assert(!sfxt.__dist[T]);
	sfxt.__dist[T] = (el == MIN) ? -(*test._rat[el][rf]) : *test._rat[el][rf];

	// shortest path dynamic programming
	_spdp(sfxt);
	// std::cout << std::endl;

	// shortest path fast algorithm
	//_spfa(sfxt);

	// relaxation from the sources
	if (_cppr_analysis) {
		auto cppr = _cppr_cache(test, el, rf);
		for (auto &[s, v] : sfxt._srcs) {
			// std::cout << sfxt.root() << " " << sfxt._srcs.size() << std::endl;
			auto [pin, srf] = _decode_pin(s);
			if (v = _cppr_offset(cppr, *pin, el, srf); v) {
				sfxt._relax(S, s, std::nullopt, *v);
			}
		}
	}
	else {
		for (auto &[s, v] : sfxt._srcs) {
			// std::cout << sfxt.root() << " " << sfxt._srcs.size() << std::endl;
			if (v = _sfxt_offset(sfxt, s); v) {

				// 每个src都参与relax
				sfxt._relax(S, s, std::nullopt, *v);
				auto [pin, vrf] = _decode_pin(s);
				std::cout << "test sfxt: " << test.related_pin().name() << " " << test.constrained_pin().name() << " "
						  << pin->name() << ": " << *v << std::endl;
			}
		}
	}
	std::cout << std::endl;

	return sfxt;
}

// Function: _sfxt_cache
SfxtCache Timer::_sfxt_cache(const Endpoint &ept) const {
	return std::visit([this, &ept](auto &&handle) { return _sfxt_cache(*handle, ept._el, ept._rf); },
					  ept._handle);
}

// Function: _my_sfxt_cache
// Find the suffix tree rooted at the super source.
SfxtCache Timer::_my_sfxt_cache(const PrimaryOutput &po, Split el, Tran rf) const {

	// 如果没有rat就报错
	assert(po._rat[el][rf]);
#ifdef _DEBUG
	std::cout << "_my_sfxt_cache po : " << *po._rat[el][rf] << std::endl;
#endif
	// _idx2pin.size()：pin的数量 * 2，因为每个pin有rise和fall
	// S只有一个，因为S不区分rise fall
	auto S = _idx2pin.size() << 1;
	// 获得po的index，作为root
	auto T = _encode_pin(po._pin, rf);

	// 以S和T的最大值初始化容器大小
	SfxtCache sfxt(el, S, T);

	// 如果刚开始source和root的dist不为空，就报错
	assert(!sfxt.__dist[S]);
	assert(!sfxt.__dist[T]);

	// MAX:相当于所有的节点的dist都增加了一个 rat
	// MIN:相当于所有的节点的dist都增加了一个-rat
	// 给source的初始dist设置为rat，这样最后计算下来root的dist就是slack
	// start at the source
	sfxt.__dist[S] = (el == MIN) ? -(*po._rat[el][rf]) : *po._rat[el][rf];

	assert(sfxt._pins.empty()); // 如果一开始_pins不为空，就报错
	// 按照从root的反向拓补DFS顺序为sfxt._pins添加所有pin
	// 保证了从前向后，所有的pin都在该pin的fan-in后面
	_topologize(sfxt, T);
	assert(!sfxt._pins.empty());

	// debug
	// for (unsigned long v : sfxt._pins) {
	// 	auto [pin, vrf] = _decode_pin(v);
	// 	std::cout << pin->name() << std::endl;
	// }
	// std::cout << std::endl;


	// 从source开始往root遍历，所有的节点的依赖关系都是可以保证的
	for (auto itr = sfxt._pins.begin(); itr != sfxt._pins.end(); ++itr) {

		// u是新建立的生成树的边的头，v是尾，与arc的from to是相反的
		auto u = *itr;
		auto      [pin, urf] = _decode_pin(u);
#ifdef _DEBUG
		std::cout << pin->name() << ":{" << std::endl;
#endif
		// Start at the data source
		if (pin->is_datapath_source()) {
			if (auto val = _sfxt_offset(sfxt, u); val) {
				sfxt._relax(u, S, std::nullopt, *val);
				// auto [pin, urf] = _decode_pin(u);
				// std::cout << "po sfxt: " << po._pin.name() << " " << pin->name() << ": " << *u << std::endl;
			}
			continue;
		}
		// Relax on fanin
		for (auto arc : pin->_fanin) {
			// 建图,建立所有的边
			FOR_EACH_RF_IF(vrf, arc->_delay[el][vrf][urf]) {
					auto v = _encode_pin(arc->_from, vrf);
					assert(sfxt.__dist[v]);
					// std::cout << '\t' << arc->_from.name() << ": " << *sfxt.__dist[v] << std::endl;
					auto d = (el == MIN) ? *arc->_delay[el][vrf][urf] : -(*arc->_delay[el][vrf][urf]);
					sfxt._relax(u, v, _encode_arc(*arc, vrf, urf), d);
					// if (sfxt._relax(u, v, _encode_arc(*arc, vrf, urf), d))
					// 	std::cout << '\t' << "from: " << arc->_from.name() << ": " << *sfxt.__dist[v] <<
					// 			  " to: " << arc->_to.name() << ": " << *sfxt.__dist[u]
					// 			  << " delay: " << d << std::endl;
				}
		}
#ifdef _DEBUG
		std::cout << "}" << std::endl;
#endif
	}

	return sfxt;
}

// Function: _sfxt_cache
// Find the suffix tree rooted at the test
SfxtCache Timer::_my_sfxt_cache(const Test &test, Split el, Tran rf) const {

	// 如果没有rat就报错
	assert(test._rat[el][rf]);
#ifdef _DEBUG
	std::cout << "_my_sfxt_cache test : " << *test._rat[el][rf] << std::endl;
#endif
	auto      S = _idx2pin.size() << 1;
	auto      T = _encode_pin(test._arc._to, rf);
	SfxtCache sfxt(el, S, T);


	// 如果刚开始source和root的dist不为空，就报错
	assert(!sfxt.__dist[S]);
	assert(!sfxt.__dist[T]);
	// MAX:相当于所有的节点的dist都增加了一个 rat
	// MIN:相当于所有的节点的dist都增加了一个-rat
	// 给source的初始dist设置为rat，这样最后计算下来root的dist就是slack
	sfxt.__dist[S] = (el == MIN) ? -(*test._rat[el][rf]) : *test._rat[el][rf];

	assert(sfxt._pins.empty()); // 如果一开始_pins不为空，就报错
	// 按照从root的反向拓补DFS顺序为sfxt._pins添加所有pin
	_topologize(sfxt, T);
	assert(!sfxt._pins.empty());
	// std::cout<<std::endl;

	for (auto itr = sfxt._pins.begin(); itr != sfxt._pins.end(); ++itr) {

		auto u = *itr;
		auto      [pin, urf] = _decode_pin(u);
#ifdef _DEBUG
		std::cout << pin->name() << ":{" << std::endl;
#endif
		if (pin->is_datapath_source()) {
			if (_cppr_analysis) {
				auto cppr = _cppr_cache(test, el, rf);

				if (auto val = _cppr_offset(cppr, *pin, el, urf); val) {
					sfxt._relax(u, S, std::nullopt, *val);
				}
			}
			else {
				if (auto val = _sfxt_offset(sfxt, u); val) {
					// 每个src都参与relax
					sfxt._relax(u, S, std::nullopt, *val);
				}
			}
			continue;
		}
		// Relax on fanin
		for (auto arc : pin->_fanin) {
			// 建图,建立所有的边
			FOR_EACH_RF_IF(vrf, arc->_delay[el][vrf][urf]) {
					auto v = _encode_pin(arc->_from, vrf);
					assert(sfxt.__dist[v]);
					// std::cout << '\t' << arc->_from.name() << ": " << *sfxt.__dist[v] << std::endl;
					auto d = (el == MIN) ? *arc->_delay[el][vrf][urf] : -(*arc->_delay[el][vrf][urf]);
					sfxt._relax(u, v, _encode_arc(*arc, vrf, urf), d);
					// if (sfxt._relax(u, v, _encode_arc(*arc, vrf, urf), d))
					// 	std::cout << '\t' << "from: " << arc->_from.name() << ": " << *sfxt.__dist[v] <<
					// 			  " to: " << arc->_to.name() << ": " << *sfxt.__dist[u]
					// 			  << " delay: " << d << std::endl;
				}
		}
#ifdef _DEBUG
		std::cout << "}" << std::endl;
#endif
	}

	return sfxt;
}

// Function: _sfxt_cache
SfxtCache Timer::_my_sfxt_cache(const Endpoint &ept) const {
	return std::visit([this, &ept](auto &&handle) { return _my_sfxt_cache(*handle, ept._el, ept._rf); },
					  ept._handle);
}

// Function: _sfxt_offset
// 构建虚拟边
std::optional<float> Timer::_sfxt_offset(const SfxtCache &sfxt, size_t v) const {

	auto [pin, rf] = _decode_pin(v);

	if (auto at = pin->_at[sfxt._el][rf]; at) {
		// 所有的节点都增加了一个rat，相当于所有的虚拟边=rat-at/-rat+at
		return sfxt._el == MIN ? *at : -*at;
	}
	else {
		return std::nullopt;
	}
}

}; // end of namespace ot. -----------------------------------------------------------------------
