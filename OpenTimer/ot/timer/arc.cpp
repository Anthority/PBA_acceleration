#include <ot/timer/arc.hpp>
#include <ot/timer/pin.hpp>
#include <ot/timer/net.hpp>

namespace ot
{

	// Constructor
	Arc::Arc(Pin &from, Pin &to, Net &net) : _from{from},
											 _to{to},
											 _handle{&net}
	{
	}

	// Constructor
	Arc::Arc(Pin &from, Pin &to, TimingView t) : _from{from},
												 _to{to},
												 _handle{t}
	{
	}

	// Function: name
	std::string Arc::name() const
	{
		return _from._name + "->" + _to._name;
	}

	// Function: is_self_loop
	bool Arc::is_self_loop() const
	{
		return &_from == &_to;
	}

	// Function: is_loop_breaker
	bool Arc::is_loop_breaker() const
	{
		return _has_state(LOOP_BREAKER);
	}

	// Function: is_net_arc
	bool Arc::is_net_arc() const
	{
		return std::get_if<Net *>(&_handle) != nullptr;
	}

	// Function: is_cell_arc
	bool Arc::is_cell_arc() const
	{
		return std::get_if<TimingView>(&_handle) != nullptr;
	}

	// Function: is_tseg
	bool Arc::is_tseg() const
	{
		if (auto ptr = std::get_if<TimingView>(&_handle); ptr)
		{
			return (*ptr)[MIN]->is_constraint();
		}
		else
			return false;
	}

	// Function: is_pseg
	bool Arc::is_pseg() const
	{
		if (auto ptr = std::get_if<TimingView>(&_handle); ptr)
		{
			return !(*ptr)[MIN]->is_constraint();
		}
		else
			return false;
	}

	// Function: timing_view
	TimingView Arc::timing_view() const
	{
		if (auto tv = std::get_if<TimingView>(&_handle); tv)
		{
			return *tv;
		}
		else
			return {nullptr, nullptr};
	}

	// Procedure: _remap_timing
	void Arc::_remap_timing(Split el, const Timing &timing)
	{
		(std::get<TimingView>(_handle))[el] = &timing;
	}

	// Procedure: _reset_delay
	void Arc::_reset_delay()
	{
		FOR_EACH_EL_RF_RF(el, frf, trf)
		{
			_delay[el][frf][trf].reset();
			_ipower[el][frf][trf].reset();
		}
	}

	extern size_t net_size;
	extern size_t net_recal_num;
	extern size_t cell_recal_num;
	extern size_t net_pba_timing_elapsed_time;
	extern size_t cell_pba_timing_elapsed_time;

	std::pair<std::optional<float>, std::optional<float>> Arc::_get_pba_timing(Split el, Tran frf, Tran trf, float si)
	{
		std::optional<float> slew, delay;
		std::visit(Functors{// Case 1: Net arc
							[&](Net *net)
							{
								net_size += net->rct()->num_nodes();
								net_recal_num += 1;
								
								auto start = std::chrono::high_resolution_clock::now();

								/* *************************************************************************************************************** */
								/*
								Rct *rct = std::get_if<Rct>(&net->_rct); // #nodes,+1是加上了input port的数量,不考虑电感的情况下
								if (!net->_C_G_initialized)
								{
									int n = rct->num_nodes() + 1;

									MatrixXd C(n, n); // capacitance matrix
									MatrixXd G(n, n); // conductance matrix
									C.setZero();
									G.setZero();

									std::unordered_map<const RctNode *, size_t> temp_node_idx;
									temp_node_idx[rct->_root] = 0;
									for (auto &[name, node] : rct->_nodes)
									{
										if (&node == rct->_root)
											continue;
										size_t temp = temp_node_idx.size();
										temp_node_idx[&node] = temp;
										C(temp, temp) = node.cap(el, frf);
									}
									for (auto &[name, node] : rct->_nodes)
									{
										size_t temp = temp_node_idx[&node];
										for (auto &edge : node._fanout)
										{
											G(temp, temp) += 0.001 / edge->res();
											size_t net_to = temp_node_idx[&(edge->_to)];
											G(temp, net_to) += -0.001 / edge->res();
										}
									}
									G(n - 1, 0) = -1.0;
									G(0, n - 1) = 1.0;

									// printMatrixXd(C);
									// printMatrixXd(G);

									int N = net->num_pins(); // N是所有pins的数量，包含root，是所有load cell的数量+1

									MatrixXd B(n, N); // input port-to-node connectivity matrix
									MatrixXd D(n, N); // output port-to-node connectivity matrix
									B.setZero();
									D.setZero();

									B(n - 1, 0) = -1.0; // 设置最后一行的第一个元素为-1，对应电压输入

									for (Pin *_pin : net->_pins)
									{
										// 获得load pin在所有node中的index
										size_t arc_to = temp_node_idx[rct->node(_pin->_name)];
										if (arc_to == 0)
											continue;

										// 获得load pin在所有pins中的index
										size_t temp = net->_load_idx.size();
										net->_load_idx[rct->node(_pin->_name)] = temp + 1;
										B(arc_to, temp + 1) = 1.0;
									}
									D = B;
									// printMatrixXd(B);

									if (n / 2 >= 4 * N)
									{
										Prima(net->_C, net->_G, net->_B, net->_D, C, G, B, D, 4);
									}
									else
									{
										net->_C = C;
										net->_G = G;
										net->_B = B;
										net->_D = D;
									}
									net->_C_G_initialized = true;
								}

								// 仿真求解
								float voltage = 1;
								// 仿真两倍的si时间，精度是si的单位的1/1000
								size_t time_size = std::ceil(si * 1000);

								int N = net->num_pins();	  // N是所有pins的数量，包含root，是所有load cell的数量+1
								MatrixXd u(N, 2 * time_size); // input vector, 电压的时间序列
								u.setZero();
								float dv = 1.0 / time_size * voltage;
								for (size_t i = 0; i < 2 * time_size; ++i)
								{
									u(0, i) = i * dv <= voltage ? i * dv : voltage;
								}

								std::vector<double> y(2 * time_size); // output vector, 输出电压的时间序列
								rc_tr_sim_d(net->_C, net->_G, net->_B, net->_D, u, 1, y, net->_load_idx[rct->node(_to._name)]);

								size_t t_10 = std::lower_bound(y.begin(), y.end(), 0.1 * voltage) - y.begin();
								size_t t_90 = std::lower_bound(y.begin(), y.end(), 0.9 * voltage) - y.begin();
								slew = (t_90 - t_10) * 1.25 / 1000.0;

								size_t t_50 = std::lower_bound(y.begin(), y.end(), 0.5 * voltage) - y.begin();
								delay = (t_50 - time_size / 2) / 1000.0;

								// std::cout<<"Slew ELM:"<< *(net->_slew(el, frf, si, _to))<<" Arnoldi:"<<*slew<<std::endl;
								// std::cout<<"Delay ELM:"<< *(net->_delay(el, frf, _to))<<" Arnoldi:"<<*delay<<std::endl;
								*/
								/* *************************************************************************************************************** */

								// 重新运行三次Elmore延时计算，来模拟Arnoldi方法的用时
								// for (int i = 0; i < 3; i++)
								// net->_recal_rc_pba_timing();
								slew = net->_slew(el, frf, si, _to);
								delay = net->_delay(el, frf, _to);

								/* *************************************************************************************************************** */

								auto end = std::chrono::high_resolution_clock::now();
								auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
								net_pba_timing_elapsed_time += duration.count();
							},
							// Case 2: Cell arc
							[&](TimingView tv)
							{
								cell_recal_num += 1;
								auto start = std::chrono::high_resolution_clock::now();

								auto lc = (_to._net) ? _to._net->_load(el, trf) : 0.0f;
								if ((tv[el] && _from._slew[el][frf]))
								{
									slew = tv[el]->slew(frf, trf, si, lc);
									delay = tv[el]->delay(frf, trf, si, lc);
								}

								auto end = std::chrono::high_resolution_clock::now();
								auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
								cell_pba_timing_elapsed_time += duration.count();
							}},
				   _handle);
		return std::make_pair(slew, delay);
	}

	// Procedure: _fprop_slew
	void Arc::_fprop_slew()
	{
		if (_has_state(LOOP_BREAKER))
		{
			return;
		}

		std::visit(Functors{// Case 1: Net arc
							[this](Net *net)
							{
								FOR_EACH_EL_RF(el, rf)
								{
									if (_from._slew[el][rf])
									{
										if (auto so = net->_slew(el, rf, *(_from._slew[el][rf]), _to); so)
										{
											_to._relax_slew(this, el, rf, el, rf, *so);
										}
									}
								}
							},
							// Case 2: Cell arc
							[this](TimingView tv)
							{
								FOR_EACH_EL_RF_RF_IF(el, frf, trf, (tv[el] && _from._slew[el][frf]))
								{
									auto lc = (_to._net) ? _to._net->_load(el, trf) : 0.0f;
									if (auto so = tv[el]->slew(frf, trf, *_from._slew[el][frf], lc); so)
									{
										_to._relax_slew(this, el, frf, el, trf, *so);
									}
								}
							}},
				   _handle);
	}

	// Procedure: _fprop_delay
	void Arc::_fprop_delay()
	{
		if (_has_state(LOOP_BREAKER))
		{
			return;
		}

		std::visit(Functors{// Case 1: Net arc
							[this](Net *net)
							{
								FOR_EACH_EL_RF(el, rf)
								{
									_delay[el][rf][rf] = net->_delay(el, rf, _to);
								}
							},
							// Case 2: Cell arc
							[this](TimingView tv)
							{
								FOR_EACH_EL_RF_RF_IF(el, frf, trf, (tv[el] && _from._slew[el][frf]))
								{
									auto lc = (_to._net) ? _to._net->_load(el, trf) : 0.0f;
									auto si = *_from._slew[el][frf];
									auto delay = tv[el]->delay(frf, trf, si, lc);
									_delay[el][frf][trf] = delay;
									auto ipower = tv[el]->internal_power.power(frf, trf, si, lc);
									_ipower[el][frf][trf] = ipower;

									// std::cout << " name:" << _from._name << " delay:" << *delay << " slew:" << si << " lc:" << lc << " ipower:" << *ipower << "\n";
								}
							}},
				   _handle);
	}

	// Procedure: _fprop_at
	void Arc::_fprop_at()
	{
		if (_has_state(LOOP_BREAKER))
		{
			return;
		}

		FOR_EACH_EL_RF_RF_IF(el, frf, trf, _from._at[el][frf] && _delay[el][frf][trf])
		{
			_to._relax_at(this, el, frf, el, trf, *_delay[el][frf][trf] + *_from._at[el][frf]);
		}
	}

	// Procedure: _bprop_rat
	void Arc::_bprop_rat()
	{
		if (_has_state(LOOP_BREAKER))
		{
			return;
		}

		std::visit(Functors{// Case 1: Net arc
							[this](Net *net)
							{
								FOR_EACH_EL_RF_IF(el, rf, _to._rat[el][rf] && _delay[el][rf][rf])
								{
									_from._relax_rat(this, el, rf, el, rf, *_to._rat[el][rf] - *_delay[el][rf][rf]);
								}
							},
							// Case 2: Cell arc
							[this](TimingView tv)
							{
								FOR_EACH_EL_RF_RF_IF(el, frf, trf, tv[el])
								{

									// propagation arc
									if (!tv[el]->is_constraint())
									{
										if (!_to._rat[el][trf] || !_delay[el][frf][trf])
										{
											continue;
										}
										_from._relax_rat(this, el, frf, el, trf, *_to._rat[el][trf] - *_delay[el][frf][trf]);
									}
									// constraint arc
									else
									{

										if (!tv[el]->is_transition_defined(frf, trf))
										{
											continue;
										}

										if (el == MIN)
										{
											auto at = _from._at[MAX][frf];
											auto slack = _to.slack(MIN, trf);
											if (at && slack)
											{
												_from._relax_rat(this, MAX, frf, MIN, trf, *at + *slack);
											}
										}
										else
										{
											auto at = _from._at[MIN][frf];
											auto slack = _to.slack(MAX, trf);
											if (at && slack)
											{
												_from._relax_rat(this, MIN, frf, MAX, trf, *at - *slack);
											}
										}
									}
								}
							}},
				   _handle);
	}

	// Procedure: _remove_state
	void Arc::_remove_state(int s)
	{
		if (s == 0)
			_state = 0;
		else
		{
			_state &= ~s;
		}
	}

	// Procedure: _insert_state
	void Arc::_insert_state(int s)
	{
		_state |= s;
	}

	// Function: _has_state
	bool Arc::_has_state(int s) const
	{
		return _state & s;
	}
}; // end of namespace ot. -----------------------------------------------------------------------
