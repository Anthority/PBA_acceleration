#include <ot/timer/timer.hpp>
#include <ot/taskflow/algorithm/reduce.hpp>

// #define DEBUG

namespace ot {

// Constructor
Point::Point(const Pin &p, size_t a, Tran t, float s, float d, float pw) : pin{p},
																		   arc{a},
																		   transition{t},
																		   slew{s},
																		   delay{d},
																		   ipower{pw} {
}

// ------------------------------------------------------------------------------------------------

// Constructor
Path::Path(float slk, const Endpoint *ept) : slack{slk},
											 endpoint{ept} {
}

// Procedure: dump_tau18
void Path::dump_tau18(std::ostream &os) const {

  // std::regex replace(":");
  //
  // auto el = endpoint->split();
  // auto rf = endpoint->transition();
  //
  // os << "Endpoint: " << std::regex_replace(back().pin.name(), replace, "/") << '\n';
  // os << "Beginpoint: " << std::regex_replace(front().pin.name(), replace, "/") << '\n';
  // // os << "= Required Time " << '\n'; //TODO: ignore RAT for tau18 benchmark
  // float rat = 0.0;
  // if (endpoint->test() != nullptr) {
  // 	rat = *(endpoint->test()->rat(el, rf));
  // }
  // else {
  // 	rat = *(endpoint->primary_output()->rat(el, rf));
  // }
  // auto beg_at     = front().at;
  // auto end_at     = back().at;
  // auto path_slack = el == MIN ? ((end_at - beg_at) - rat) : (rat - (end_at - beg_at));
  // os << "= Required Time " << rat << '\n';
  // // Arrival Time is the total delay
  // os << "- Arrival Time " << end_at - beg_at << '\n';
  // // os << "- Arrival Time " << back().at << '\n';
  // os << "= Slack Time " << path_slack << '\n';
  //
  // float                at_offset = front().at;
  // std::optional<float> pi_at;
  //
  // for (const auto &p : *this) {
  //
  // 	if (!pi_at) {
  // 		os << "- ";
  // 	}
  // 	else {
  // 		os << p.at - *pi_at << " ";
  // 	}
  // 	os << p.at - at_offset << " ";
  //
  // 	if (p.transition == RISE) {
  // 		os << "^ ";
  // 	}
  // 	else {
  // 		os << "v ";
  // 	}
  //
  // 	os << std::regex_replace(p.pin.name(), replace, "/") << '\n';
  // 	pi_at = p.at;
  // }
  // os << '\n';
}

// Procedure: dump
// dump the path in the following format:
//
// Startpoint    : inp1
// Endpoint      : f1:D
// Analysis type : early
// ------------------------------------------------------
//        Type       Delay        Time   Dir  Description
// ------------------------------------------------------
//        port       0.000       0.000  fall  inp1
//         pin       0.000       0.000  fall  u1:A (NAND2X1)
//         pin       2.786       2.786  rise  u1:Y (NAND2X1)
//         pin       0.000       2.786  rise  u4:A (NOR2X1)
//         pin       0.181       2.967  fall  u4:Y (NOR2X1)
//         pin       0.000       2.967  fall  f1:D (DFFNEGX1)
//     arrival                   2.967        data arrival time
//
//       clock      25.000      25.000  fall  f1:CLK (DFFNEGX1)
//  constraint       1.518      26.518        library hold_falling
//    required                  26.518        data required time
// ------------------------------------------------------
//       slack                 -23.551        VIOLATED
//
void Path::dump(std::ostream &os) const {

  if (empty()) {
	os << "empty path\n";
	return;
  }

  auto fmt = os.flags();
  auto split = endpoint->split();
  auto tran = endpoint->transition();
  // auto at    = back().pin.;
  // auto rat   = (split == MIN ? at - slack : at + slack);

  // Print the head
  os << "Startpoint    : " << front().pin.name() << '\n';
  os << "Endpoint      : " << back().pin.name() << '\n';
  os << "Analysis type : " << to_string(split) << '\n';

  size_t w1 = 11;
  size_t w_slew = 12;
  size_t w2 = 12;
  size_t w3 = 12;
  size_t w4 = 6;
  size_t w5 = 13;
  size_t W = w1 + w_slew + w2 + w3 + w4 + w5;

  std::fill_n(std::ostream_iterator<char>(os), W, '-');
  os << '\n'
	 << std::setw(w1) << "Type"
	 << std::setw(w_slew) << "Slew"
	 << std::setw(w2) << "Delay"
	 << std::setw(w3) << "Time"
	 << std::setw(w4) << "Dir";
  std::fill_n(std::ostream_iterator<char>(os), 2, ' ');
  os << "Description" << '\n';
  std::fill_n(std::ostream_iterator<char>(os), W, '-');
  os << '\n';

  // trace
  os << std::fixed << std::setprecision(6);
  // std::optional<float> pi_at;
  float at = 0;

  for (const auto &p : *this) {

	// type
	if (p.pin.primary_input() || p.pin.primary_output()) {
	  os << std::setw(w1) << "port";
	} else {
	  os << std::setw(w1) << "pin";
	}

	// slew
	os << std::setw(w_slew);
	os << p.slew;

	// delay
	os << std::setw(w2) << p.delay;

	// if (pi_at)
	// 	os << p.at - *pi_at;
	// else
	// 	os << p.at;

	// arrival time
	at += p.delay;
	os << std::setw(w3) << at;

	// internal power
	// ？？？没输出
	// os << std::setw(w3) << p.ipower;

	// transition
	os << std::setw(w4) << to_string(p.transition);

	// pin name
	std::fill_n(std::ostream_iterator<char>(os), 2, ' ');
	if (os << p.pin.name(); p.pin.gate()) {
	  os << " (" << p.pin.gate()->cell_name() << ')';
	}
	os << '\n';

	// cursor
	// pi_at = p.at;
  }

  os << std::setw(w1) << "arrival"
	 << std::setw(w_slew + w2 + w3) << at;
  std::fill_n(std::ostream_iterator<char>(os), w4 + 2, ' ');
  os << "data arrival time" << '\n';

  // Print the required arrival time
  os << '\n';

  auto rat = (split == MIN ? at - slack : at + slack);

  // test type
  std::visit(Functors{[&](Test *test) {
			   auto tv = (test->_arc.timing_view())[split];
			   auto sum = 0.0f;

			   // related pin latency
			   os << std::setw(w1) << "related pin";
			   if (auto c = test->_related_at[split][tran]; c) {
				 sum += *c;
				 os << std::setw(w_slew + w2) << *c << std::setw(w3) << sum;
			   } else {
				 os << std::setw(w_slew + w2 + w3) << "n/a";
			   }

			   if (tv && tv->is_rising_edge_triggered()) {
				 os << std::setw(w4) << "rise";
			   } else if (tv && tv->is_falling_edge_triggered()) {
				 os << std::setw(w4) << "fall";
			   } else {
				 os << "n/a";
			   }

			   std::fill_n(std::ostream_iterator<char>(os), 2, ' ');
			   if (os << test->related_pin().name(); test->related_pin().gate()) {
				 os << " (" << test->related_pin().gate()->cell_name() << ')';
			   }
			   os << '\n';

			   // constraint value
			   os << std::setw(w1) << "constraint";
			   if (auto c = test->_constraint[split][tran]; c) {

				 switch (split) {
				   case MIN: sum += *c;
					 os << std::setw(w_slew + w2) << c.value() << std::setw(w3) << sum;
					 break;

				   case MAX: sum -= *c;
					 os << std::setw(w_slew + w2) << -c.value() << std::setw(w3) << sum;
					 break;
				 }

				 // timing type
				 if (tv && tv->type) {
				   std::fill_n(std::ostream_iterator<char>(os), w4 + 2, ' ');
				   os << "library " << to_string(tv->type.value()) << '\n';
				 } else {
				   os << '\n';
				 }
			   } else {
				 os << std::setw(w2) << "n/a" << '\n';
			   }

			   // cppr credit
			   if (auto c = test->_cppr_credit[split][tran]; c) {
				 os << std::setw(w1) << "cppr credit";
				 sum += *c;
				 os << std::setw(w_slew + w2) << *c << std::setw(w3) << sum << '\n';
			   }

			   OT_LOGW_IF(
				   std::fabs(sum - rat) > 1.0f,
				   "unstable numerics in PBA and GBA rats: ", sum, " vs ", rat);
			 },
				 [&](PrimaryOutput *po) {
				   os << std::setw(w1) << "port";
				   if (auto v = po->rat(split, tran); v) {
					 os << std::setw(w_slew + w2) << *v << std::setw(w3) << *v;
					 std::fill_n(std::ostream_iterator<char>(os), w4 + 2, ' ');
					 os << "output port delay" << '\n';
				   } else {
					 os << std::setw(w_slew + w2) << "n/a" << '\n';
				   }
				 }},
			 endpoint->_handle);

  os << std::setw(w1) << "required" << std::setw(w_slew + w2 + w3) << rat;
  std::fill_n(std::ostream_iterator<char>(os), w4 + 2, ' ');
  os << "data required time" << '\n';

  // slack
  std::fill_n(std::ostream_iterator<char>(os), W, '-');
  os << '\n'
	 << std::setw(w1) << "slack" << std::setw(w_slew + w2 + w3) << slack;
  std::fill_n(std::ostream_iterator<char>(os), w4 + 2, ' ');
  os << (slack < 0.0f ? "VIOLATED" : "MET") << '\n';

  // restore the format
  os.flags(fmt);
}

// Operator <<
std::ostream &operator<<(std::ostream &os, const Path &path) {
  path.dump(os);
  return os;
}

// ------------------------------------------------------------------------------------------------

// Functoin: _extract
// Extract the path in ascending order.
std::vector<Path> PathHeap::extract() {
  std::sort_heap(_paths.begin(), _paths.end(), _comp);
  std::vector<Path> P;
  P.reserve(_paths.size());
  std::transform(_paths.begin(), _paths.end(), std::back_inserter(P), [](auto &ptr) { return std::move(*ptr); });
  _paths.clear();
  return P;
}

// Procedure: push
void PathHeap::push(std::unique_ptr<Path> path) {
  _paths.push_back(std::move(path));
  std::push_heap(_paths.begin(), _paths.end(), _comp);
}

// Procedure: pop
void PathHeap::pop() {
  if (_paths.empty()) {
	return;
  }
  std::pop_heap(_paths.begin(), _paths.end(), _comp);
  _paths.pop_back();
}

// Function: top
Path *PathHeap::top() const {
  return _paths.empty() ? nullptr : _paths.front().get();
}

// Procedure: fit
void PathHeap::fit(size_t K) {
  while (_paths.size() > K) {
	pop();
  }
}

// Procedure: heapify
void PathHeap::heapify() {
  std::make_heap(_paths.begin(), _paths.end(), _comp);
}

// Procedure: merge_and_fit
void PathHeap::merge_and_fit(PathHeap &&rhs, size_t K) {

  if (_paths.capacity() < rhs._paths.capacity()) {
	_paths.swap(rhs._paths);
  }

  std::sort_heap(_paths.begin(), _paths.end(), _comp);
  std::sort_heap(rhs._paths.begin(), rhs._paths.end(), _comp);

  auto mid = _paths.insert(
	  _paths.end(),
	  std::make_move_iterator(rhs._paths.begin()),
	  std::make_move_iterator(rhs._paths.end()));

  rhs._paths.clear();

  std::inplace_merge(_paths.begin(), mid, _paths.end(), _comp);

  if (_paths.size() > K) {
	_paths.resize(K);
  }

  heapify();
}

// Function: dump
std::string PathHeap::dump() const {
  std::ostringstream oss;
  oss << "# Paths: " << _paths.size() << '\n';
  for (size_t i = 0; i < _paths.size(); ++i) {
	oss << "slack[" << i << "]: " << _paths[i]->slack << '\n';
  }
  return oss.str();
}

// ------------------------------------------------------------------------------------------------

// Function: report_timing
// Report the top-k report_timing
std::vector<Path> Timer::report_timing(size_t K) {
  std::scoped_lock lock(_mutex);
  return _report_timing(_worst_endpoints(K), K);
}

std::vector<std::unique_ptr<Path>> Timer::my_report_timing(size_t K) {
  std::scoped_lock lock(_mutex);
  return _my_report_timing(_worst_endpoints(K), K);
}

// Function: report_timing
std::vector<Path> Timer::report_timing(size_t K, Split el) {
  std::scoped_lock lock(_mutex);
  return _report_timing(_worst_endpoints(K, el), K);
}

// Function: report_timing
std::vector<Path> Timer::report_timing(size_t K, Tran rf) {
  std::scoped_lock lock(_mutex);
  return _report_timing(_worst_endpoints(K, rf), K);
}

// Function: report_timing
std::vector<Path> Timer::report_timing(size_t K, Split el, Tran rf) {
  std::scoped_lock lock(_mutex);
  return _report_timing(_worst_endpoints(K, el, rf), K);
}

// TODO (Guannan)
// Function: report_timing
std::vector<Path> Timer::report_timing(PathGuide guide) {
  std::scoped_lock lock(_mutex);
  auto epts = _worst_endpoints(guide);
  return {};
}

// Function: _report_timing
// Report the top-k report_timing
std::vector<Path> Timer::_report_timing(std::vector<Endpoint *> &&epts, size_t K) {

  assert(epts.size() <= K);

  // No need to report anything.
  if (K == 0 || epts.empty()) {
	return {};
  }

  // No need to generate prefix tree
  if (K == 1) {
	std::vector<Path> paths;
	paths.emplace_back(epts[0]->slack(), epts[0]);
	auto sfxt = _sfxt_cache(*epts[0]);

	OT_LOGW_IF(
		std::fabs(*sfxt.slack() - paths[0].slack) > 1.0f,
		"unstable numerics in PBA and GBA slacks: ", *sfxt.slack(), " vs ", paths[0].slack);

	// assert(std::fabs(*sfxt.slack() - paths[0].slack) < 0.1f);
	_recover_datapath(paths[0], sfxt);
	return paths;
  }

  /* 多线程中对beg和end之间的每个元素应用bop和uop操作，并将结果累加到init中
   * uop是一个一元操作，它接受一个元素作为输入，并返回一个结果。在transform_reduce函数中，uop被应用到输入范围beg和end之间的每个元素上。
   * bop是一个二元操作，它接受两个元素作为输入，并返回一个结果。在transform_reduce函数中，bop被用来将uop的结果累加到init中。
   * 对于输入范围beg和end之间的每个元素，首先应用uop操作。
   * 然后，将uop的结果使用bop操作累加到init中。
   * 这个过程在多线程中进行，每个线程处理输入范围的一部分，并在完成后将结果返回。最后，使用bop操作将所有线程的结果合并。
   */
  // Generate the prefix tree
  PathHeap heap;
  _taskflow.transform_reduce(
	  epts.begin(), epts.end(), heap,
	  [&](PathHeap l, PathHeap r) mutable {
		l.merge_and_fit(std::move(r), K);
		return l;
	  },
	  [&](Endpoint *ept) {
		PathHeap heap;
		_my_spur(*ept, K, heap);
		return heap;
	  });

  _executor.run(_taskflow).wait();
  _taskflow.clear();

  return heap.extract();
}

std::vector<std::unique_ptr<Path>> Timer::_my_report_timing(std::vector<Endpoint *> &&epts, size_t K) {

  assert(epts.size() <= K);

  // Generate the prefix tree
  PathHeap heap;
  _taskflow.transform_reduce(
	  epts.begin(), epts.end(), heap,
	  [&](PathHeap l, PathHeap r) mutable {
		l.merge_and_fit(std::move(r), K);
		return l;
	  },
	  [&](Endpoint *ept) {
		PathHeap heap;
		_my_spur(*ept, K, heap);
		return heap;
	  });

  _executor.run(_taskflow).wait();
  _taskflow.clear();

  std::sort_heap(heap._paths.begin(), heap._paths.end(), heap._comp);
  std::vector<std::unique_ptr<Path>> P;
  P.reserve(heap._paths.size());
  std::transform(heap._paths.begin(),
				 heap._paths.end(),
				 std::back_inserter(P),
				 [](auto &ptr) { return std::move(ptr); });
  heap._paths.clear();
  return P;
}

// Procedure: _recover_prefix
// Recover the worst path prefix at a given pin.
// void Timer::_recover_prefix(Path &path, const SfxtCache &sfxt, size_t idx) const {
//
// 	auto el = sfxt._el;
// 	auto [v, rf] = _decode_pin(idx);
//
// 	assert(v->_at[el][rf]);
//
// 	path.emplace_front(*v, rf, *v->_at[el][rf], *v->_slew[el][rf], 0.0);
//
// 	if (auto arc = v->_at[el][rf]->pi_arc; arc) {
// 		_recover_prefix(path, sfxt, _encode_pin(arc->_from, v->_at[el][rf]->pi_rf));
// 	}
// }

// Procedure: _recover_datapath
// Recover the worst data path from a given suffix tree.
void Timer::_recover_datapath(Path &path, const SfxtCache &sfxt) const {

  if (!sfxt.__tree[sfxt._S]) {
	return;
  }
  // 寻找父节点
  auto u = *sfxt.__tree[sfxt._S];
  auto [upin, urf] = _decode_pin(u);

  // data path source
  assert(upin->_at[sfxt._el][urf]);
  path.emplace_back(*upin, 0, urf, *upin->_slew[sfxt._el][urf], 0.0, 0.0);

  // recursive
  while (u != sfxt._T) {
	assert(sfxt.__link[u]);
	auto [arc, frf, trf] = _decode_arc(*sfxt.__link[u]);
	u = *sfxt.__tree[u];
	std::tie(upin, urf) = _decode_pin(u);
	assert(path.back().transition == frf && urf == trf);
	auto delay = *arc->_delay[sfxt._el][frf][trf];
	auto ip = *arc->_ipower[sfxt._el][frf][trf];
	path.emplace_back(*upin, 0, urf, *arc->to().slew(sfxt._el, trf), delay, ip);
  }
}

// Procedure: _recover_datapath
// recover the data path from a given prefix tree node w.r.t. a suffix tree
void Timer::_recover_datapath(Path &path, const SfxtCache &sfxt, const PfxtNode *node, size_t v) const {

  if (node == nullptr) {
	return;
  }

  // devation node's from is the end of the parent path segment
  _recover_datapath(path, sfxt, node->parent, node->from);

  // devation node's to is the start of the path segment
  auto u = node->to;
  auto [upin, urf] = _decode_pin(u);

  // data path source deviation
  if (node->from == sfxt._S) {
	assert(upin->_at[sfxt._el][urf]);
	// 初始化data path source的at
	path.emplace_back(*upin, 0, urf, *upin->_slew[sfxt._el][urf], 0.0, 0.0);
  }

	// internal deviation: 需要算一下这个pin的新的at
  else {
	assert(!path.empty());
	// To Change: 没必要记录at，需要保留的是每一段时序弧的delay
	auto delay = *node->arc->_delay[sfxt._el][path.back().transition][urf];
	auto ip = *node->arc->_ipower[sfxt._el][path.back().transition][urf];
	// ？？？？？？？？？？？
	path.emplace_back(*upin, 0, urf, *upin->_slew[sfxt._el][urf], delay, ip);
  }

  // 在最短路径树上恢复
  while (u != v) {
	assert(sfxt.__link[u]);
	auto [arc, frf, trf] = _decode_arc(*sfxt.__link[u]);
	u = *sfxt.__tree[u];
	std::tie(upin, urf) = _decode_pin(u);
	assert(path.back().transition == frf && urf == trf);
	auto delay = *arc->_delay[sfxt._el][frf][trf];
	auto ip = *arc->_ipower[sfxt._el][frf][trf];
	path.emplace_back(*upin, 0, urf, *upin->_slew[sfxt._el][urf], delay, ip);
  }
}

void Timer::_my_recover_datapath(Path &path, const SfxtCache &sfxt, const PfxtNode *node, size_t stop) const {
  // std::unique_ptr<Path> pathptr,

  if (node == nullptr) {
	return;
  }

  // child's from is the end of the path segment
  // and do not add it into that path segment
  _my_recover_datapath(path, sfxt, node->parent, node->from);

  // 添加偏离边到path
  // devation node's from is the start of the path segment
  auto u = node->from; // po/test
  auto [upin, urf] = _decode_pin(u);
  assert(u != sfxt._T || (path.empty() && sfxt.__dist[u]));

  auto v = node->to;
  auto [vpin, vrf] = _decode_pin(v);

  auto arc = *node->arc;

  auto delay = *arc._delay[sfxt._el][vrf][urf];
  auto ip = *arc._ipower[sfxt._el][vrf][urf];

  path.emplace_front(*upin, _encode_arc(arc, vrf, urf), urf, *upin->_slew[sfxt._el][urf], delay, ip);

  // std::cout << "start: " << upin->name() << " :{" << std::endl;
  // std::cout << '\t' << "from: " << node->arc->_to.name() << ": " << *sfxt.__dist[u] << " to: "
  // 		  << node->arc->_from.name() << " delay: " << delay
  // 		  << std::endl;

  size_t i = 0;
  // 在最短路径树上恢复路径
  while (v != stop) {
	i++;

	u = v; // u走向该路径段的真正起始node的to
	std::tie(upin, urf) = _decode_pin(u);

	v = *sfxt.__tree[u];
	std::tie(vpin, vrf) = _decode_pin(v);

	if (v == sfxt._S) {
	  // 因为这个arc是虚拟边，无法解析
	  path.emplace_front(*upin, INTMAX_MAX, urf, *upin->_slew[sfxt._el][urf], *upin->_at[sfxt._el][urf], 0);
	  if (node->parent) {
		path.parent = node->parent->path;
		path.parent_idx = path.parent->size() - 1 - node->parent_idx;
		// std::cout << path.parent_idx << std::endl;
	  }
	  path.dev_idx = i;
	  break;
#ifdef  _DEBUG
	  std::cout << "_my_recover_datapath: " << upin->name() << " " << path.parent << " " << path.dev_idx
				<< std::endl;
#endif
	}
	auto [arc, frf, trf] = _decode_arc(*sfxt.__link[u]);
	assert(urf == trf && vrf == frf);

	delay = *arc->_delay[sfxt._el][vrf][urf];
	ip = *arc->_ipower[sfxt._el][vrf][urf];
	path.emplace_front(*upin, *sfxt.__link[u], urf, *upin->_slew[sfxt._el][urf], delay, ip);
  }
}

size_t recal_num = 0;
float Timer::cal_arc_pba_timing(Point &node_from, Point &node_to, Split el) {
  recal_num++;
  float slack_credit = 0;

  auto frf = node_from.transition;
  auto trf = node_to.transition;
  for (auto &arc : node_to.pin._fanin) {
	assert(node_to.arc != INTMAX_MAX);
	if (_encode_arc(*arc, frf, trf) == node_to.arc) {
	  auto pin_slew = arc->_get_slew(el, frf, trf, node_from.slew);
	  assert(pin_slew);
	  node_to.slew = *pin_slew;

	  float origin_delay = node_to.delay;
	  node_to.delay = *arc->_get_delay(el, frf, trf, node_from.slew);
	  slack_credit += (origin_delay - node_to.delay);
	}
  }
  return slack_credit;
}
void Timer::report_timing_pba(std::vector<std::unique_ptr<Path>> &paths) {
  int path_num = 0;
  for (auto &path : paths) {
	std::cout << path_num++ << std::endl;

	// 如果路径的第一个pin不是data source就报错
	assert((*path).front().pin.is_datapath_source());
	auto el = (*path).endpoint->split();

	float slack_credit = 0;
	for (size_t i = 1; i < (*path).size(); i++) {
	  slack_credit += cal_arc_pba_timing((*path)[i - 1], (*path)[i], el);
	}
	(*path).slack += (el == MIN) ? -slack_credit : slack_credit;
  }
  std::cout << "RECALCULATE PATH SEGMENT NUM IN PBA MODE:" << recal_num << std::endl;
}

void Timer::new_report_timing_pba_merge(std::vector<std::unique_ptr<Path>> &paths) {
  //                 path_segment_key      slew             path    start
  std::unordered_map<std::string, std::map<float, std::pair<Path *, size_t>>> path_segments;
  int path_num = 0;
  for (auto &path : paths) {
	std::cout << "pba_merge:" << path_num++ << std::endl;
	// 如果路径的第一个pin不是data source就报错
	assert((*path).front().pin.is_datapath_source());
	auto el = (*path).endpoint->split();
	float slack_credit = 0;

	size_t last_merge;
	size_t now_merge = 0;

	for (size_t i = 0; i < (*path).size(); i++) {
	  if (((*path)[i].pin._fanin.size() > 2) || (i == (*path).size() - 1)) {
		last_merge = now_merge;
		now_merge = i;

		auto &pathseg_start = (*path)[last_merge];
		auto &pathseg_end = (*path)[now_merge - 1];

		/*
		auto key = to_string(_encode_pin((*path).front().pin, (*path).front().transition)) + "/"
			+ to_string(_encode_pin((*path)[first_merge_idx ].pin, (*path)[first_merge_idx ].transition)) + "/"
			+ to_string(el);
		*/
		auto key = to_string(pathseg_start.pin.name()) + "/"
			+ to_string(pathseg_start.transition) + "/"
			+ to_string(pathseg_end.pin.name()) + "/"
			+ to_string(pathseg_end.transition) + "/" + to_string(el);

		float min_slew_dif = 1000.0f;
		Path *path_sgement;
		size_t start_id;
		// auto [new_parent, parent_start_idx] = merge_points[key].begin()->second;
		for (auto &psg : path_segments[key]) {
		  float slew_dif = std::abs(psg.first - pathseg_start.slew);
		  if (slew_dif < min_slew_dif) {
			min_slew_dif = slew_dif;
			std::tie(path_sgement, start_id) = psg.second;
		  }
		}

		float acceptable_slew = 0.01f;
		// 如果该路径段在该Slew下的时序信息之前没有计算过
		if (min_slew_dif - acceptable_slew > 1e-9f) {
		  // 计算: last_merge ->  now_merge - 1
		  // std::cout << "\trecal:";
		  for (size_t j = last_merge + 1; j <= now_merge; j++) {
			// std::cout << '\t' << j;
			// std::cout << '\t' << (*path)[j + 1].slew;
			slack_credit += cal_arc_pba_timing((*path)[j - 1], (*path)[j], el);
			// std::cout << '\t' << (*path)[j + 1].slew;
			// std::cout << std::endl;
		  }
		  // std::cout << std::endl;
		  path_segments[key][pathseg_start.slew] = std::make_pair(path.get(), last_merge);
		} else {
		  // auto [path_sgement, start_id] = path_segments[key][pathseg_start.slew];
		  // 计算: last_merge + 1 ->  now_merge
		  std::cout << "\tmerge:";
		  for (size_t j = last_merge + 1; j <= now_merge; j++) {
			std::cout << '\t' << j;
			assert((*path)[j].pin.name() == (*path_sgement)[++start_id].pin.name());
			(*path)[j].slew = (*path_sgement)[start_id].slew;
			slack_credit += (*path)[j].delay - (*path_sgement)[start_id].delay;
			(*path)[j].delay = (*path_sgement)[start_id].delay;
		  }
		  std::cout << std::endl;
		}
	  }
	}
	(*path).slack += (el == MIN) ? -slack_credit : slack_credit;
  }
  for (auto &mp : path_segments) {
	std::cout << mp.first << std::endl;
	for (auto &map : mp.second) { // std::map<float, std::pair<Path *, size_t>>
	  std::cout << '\t' << map.first << std::endl;
	}
  }
  std::cout << "RECALCULATE PATH SEGMENT NUM IN PBA MODE:" << recal_num << std::endl;
}
void Timer::report_timing_pba_merge(std::vector<std::unique_ptr<Path>> &paths) {
  //                 path_segment_key      slew             path    idx
  std::unordered_map<std::string, std::map<float, std::pair<Path *, size_t>>> merge_points;

  int path_num = 0;
  for (auto &path : paths) {
	std::cout << "pba_merge:" << path_num++ << std::endl;
	// 如果路径的第一个pin不是data source就报错
	assert((*path).front().pin.is_datapath_source());
	auto el = (*path).endpoint->split();
	float slack_credit = 0;

	size_t first_merge_idx = 0;
	for (auto &p : (*path)) {
	  if (p.pin._fanin.size() > 2) {
		break;
	  }
	  first_merge_idx++;
	}
	/*
	  auto key = to_string(_encode_pin((*path).front().pin, (*path).front().transition)) + "/"
		  + to_string(_encode_pin((*path)[first_merge_idx ].pin, (*path)[first_merge_idx ].transition)) + "/"
		  + to_string(el);
	*/
	auto key = to_string((*path).front().pin.name()) + "/"
		+ to_string((*path).front().transition) + "/"
		+ to_string((*path)[first_merge_idx].pin.name()) + "/"
		+ to_string((*path)[first_merge_idx].transition) + "/" + to_string(el);

	// 如果找到了从datasource开始的这段路径
	if (merge_points.find(key) != merge_points.end()) {
	  auto [merge_path, merge_start_idx] = merge_points[key][(*path).front().slew];
	  assert(merge_start_idx == 0);

	  std::cout << "\tmerging: data source merge" << std::endl;
	  std::cout << "\t\t" << (*path)[0].pin.name() << " before: " << (*path)[0].slew;

	  // 计算path: 0 -> first_merge_idx
	  for (size_t i = 0; i <= first_merge_idx; i++) {
		(*path)[i].slew = (*merge_path)[i].slew;
		slack_credit += (*path)[i].delay - (*merge_path)[i].delay;
		(*path)[i].delay = (*merge_path)[i].delay;
	  }
	  std::cout << " after: " << (*merge_path)[0].slew << std::endl;
	  std::cout << "\t\t" << (*path)[first_merge_idx].pin.name() << std::endl;

	} else {
	  merge_points[key][(*path).front().slew] = (std::make_pair(path.get(), 0));
	  // 对从datasource开始的这段路径进行计算
	  auto &seg_endpoint = (*path)[(*path).dev_idx - 1];
	  for (size_t i = 1; i < (*path).dev_idx; i++) {

		// 计算path: 0 -> dev_idx - 1
		auto &node_from = (*path)[i - 1];
		auto &node_to = (*path)[i];

		slack_credit += cal_arc_pba_timing(node_from, node_to, el);

		// 先把这段最短路径树上的合并节点的 Slew 添加进 merge_points
		if (node_to.pin._fanin.size() > 2) {
		  /*
		  auto key = to_string(_encode_pin(node_to.pin, node_to.transition)) + "/"
			  + to_string(_encode_pin(seg_endpoint.pin, seg_endpoint.transition)) + "/"
			  + to_string(el);
		  */
		  auto key = to_string(node_to.pin.name()) + "/" + to_string(node_to.transition) +
			  "/" + to_string(seg_endpoint.pin.name()) + "/" + to_string(seg_endpoint.transition) + "/" + to_string(el);

		  if (merge_points[key].find(node_to.slew) == merge_points[key].end())
			merge_points[key][node_to.slew] = (std::make_pair(path.get(), i));
		}
	  }

	}

	if ((*path).parent)
	  slack_credit += report_timing_pba_merge(path.get(), (*path).dev_idx, path.get(), merge_points);
	else
	  // 计算path: new_dev_idx
	  // 当算到最后一个点时，需要为它单独计算slew和delay，因为不会再进入下一级递归来计算偏离边的属性
	  slack_credit += cal_arc_pba_timing((*path)[(*path).dev_idx - 1], (*path)[(*path).dev_idx], el);

	(*path).slack += (el == MIN) ? -slack_credit : slack_credit;
  }

  for (auto &mp : merge_points) {
	std::cout << mp.first << std::endl;
	for (auto &map : mp.second) { // std::map<float, std::pair<Path *, size_t>>
	  std::cout << '\t' << map.first << ":" << map.second.second << std::endl;
	  auto [path, path_idx] = map.second;
	  // size_t new_dev_idx = dev_idx + parent->dev_idx - path->parent_idx;
	  for (auto i = map.second.second; i < map.second.first->dev_idx; i++)
		std::cout << "\t\t" << (*map.second.first)[i].pin.name() << std::endl;
	}
  }
  std::cout << "RECALCULATE PATH SEGMENT NUM IN PBA MODE:" << recal_num << std::endl;
}
float Timer::report_timing_pba_merge(Path *ori_path,
									 size_t dev_idx,
									 Path *path,
									 std::unordered_map<
										 std::string,
										 std::map<float, std::pair<Path *, size_t>>>
									 &merge_points) {
  /*
	for (auto &mp : merge_points) {
	  std::cout << mp.first << std::endl;
	  for (auto &map : mp.second) {//std::map<float, std::pair<Path *, size_t>>
		std::cout << '\t' << map.first << ":" << map.second.second << std::endl;
		for (auto i = map.second.second; i < map.second.first->dev_idx; i++)
		  std::cout << "\t\t" << (*map.second.first)[i].pin.name() << std::endl;
	  }
	}
  */
  float slack_credit = 0;
  auto el = (*ori_path).endpoint->split();

  // 计算path: dev_idx
  auto &dev_node_from = (*ori_path)[dev_idx - 1];
  auto &dev_node_to = (*ori_path)[dev_idx];
  slack_credit += cal_arc_pba_timing(dev_node_from, dev_node_to, el);
  float now_slew = dev_node_to.slew;

  auto parent = path->parent;
  auto seg_endpoint = (*parent)[(*parent).dev_idx - 1];
  /*
  auto key = to_string(_encode_pin(dev_node_to.pin, dev_node_to.transition)) + "/"
	  + to_string(_encode_pin(seg_endpoint.pin, seg_endpoint.transition)) + "/"
	  + to_string(el);
  */
  auto key = to_string(dev_node_to.pin.name()) + "/" + to_string(dev_node_to.transition) + "/"
	  + to_string(seg_endpoint.pin.name()) + "/" + to_string(seg_endpoint.transition) + "/" + to_string(el);

  float min_slew_dif = 1000.0f;
  Path *merge_path;
  size_t merge_start_idx;
  // auto [new_parent, parent_start_idx] = merge_points[key].begin()->second;
  for (auto &mp : merge_points[key]) {
	float slew_dif = std::abs(mp.first - now_slew);
	if (slew_dif < min_slew_dif) {
	  min_slew_dif = slew_dif;
	  std::tie(merge_path, merge_start_idx) = mp.second;
	}
  }

  // 计算path: dev_idx + 1 -> new_dev_idx - 1
  float acceptable_slew = 100.0f;
  size_t new_dev_idx = dev_idx + parent->dev_idx - path->parent_idx;
  if (min_slew_dif - acceptable_slew <= 1e-9f) { // 能合并
	std::cout << "\tmerging: " << min_slew_dif << std::endl;
	std::cout << "\t\t" << (*ori_path)[dev_idx].pin.name() << " before: " << now_slew;

	// 对于dev_idx要重置其slew与delay为merge path上的slew与delay
	for (size_t i = dev_idx; i < new_dev_idx; i++) {
	  (*ori_path)[i].slew = (*merge_path)[merge_start_idx].slew;

	  slack_credit += (*ori_path)[i].delay - (*merge_path)[merge_start_idx].delay;
	  (*ori_path)[i].delay = (*merge_path)[merge_start_idx++].delay;
	}
	std::cout << " after: " << (*ori_path)[dev_idx].slew << std::endl;
	std::cout << "\t\t" << (*ori_path)[new_dev_idx - 1].pin.name() << std::endl;
  } else {
	merge_points[key].emplace(now_slew, std::make_pair(ori_path, dev_idx));
	for (size_t i = dev_idx + 1; i < new_dev_idx; i++) {
	  slack_credit += cal_arc_pba_timing((*ori_path)[i - 1], (*ori_path)[i], el);
	}
  }

  if ((*parent).parent)
	slack_credit += report_timing_pba_merge(ori_path, new_dev_idx, parent, merge_points);
  else
	// 计算path: new_dev_idx
	// 当算到最后一个点时，需要为它单独计算slew和delay，因为不会再进入下一级递归来计算偏离边的属性
	slack_credit += cal_arc_pba_timing((*ori_path)[new_dev_idx - 1], (*ori_path)[new_dev_idx], el);

  return slack_credit;
}
}; // end of namespace ot. -----------------------------------------------------------------------
