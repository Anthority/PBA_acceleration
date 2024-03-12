#include <ot/timer/endpoint.hpp>
#include <ot/timer/timer.hpp>

namespace ot
{

	// Constructor
	Endpoint::Endpoint(Split el, Tran rf, Test &test) : _el{el},
														_rf{rf},
														_handle{&test}
	{

		OT_LOGF_IF(!test.slack(el, rf), "test slack not defined");
	}

	// Constructor
	Endpoint::Endpoint(Split el, Tran rf, PrimaryOutput &po) : _el{el},
															   _rf{rf},
															   _handle{&po}
	{

		OT_LOGF_IF(!po.slack(el, rf), "PO slack not defined");
	}

	// Function: slack
	float Endpoint::slack() const
	{
		return std::visit([this](auto &&handle)
						  { return *(handle->slack(_el, _rf)); },
						  _handle);
	}

	// ------------------------------------------------------------------------------------------------

	// Function: _worst_endpoints
	std::vector<Endpoint *> Timer::_worst_endpoints(size_t K, Split el, Tran rf)
	{
		_update_endpoints();
		/*
			这段代码是使用C++的`std::transform`函数。`std::transform`函数是C++标准库中的一个算法，它可以对输入范围内的每个元素应用一个给定的函数，并将结果存储到输出范围中。
			在这段代码中，`std::transform`函数的输入范围是由`beg`和`end`两个迭代器定义的。这两个迭代器分别指向`_endpoints[el][rf]`的开始位置和结束位置。
			`std::back_inserter(epts)`是一个插入迭代器，它可以在`epts`向量的末尾插入元素。这意味着`std::transform`函数的结果将被添加到`epts`向量的末尾。
			`[](Endpoint &ept) { return &ept; }`是一个lambda函数，它接受一个`Endpoint`类型的引用，并返回这个引用的地址。这意味着`std::transform`
			函数将`_endpoints[el][rf]`中的每个元素（这里是`Endpoint`对象）的地址添加到`epts`向量中。
			总的来说，这段代码的作用是将`_endpoints[el][rf]`中的每个元素的地址添加到`epts`向量中。
		 */
		auto beg = _endpoints[el][rf].begin();
		auto end = std::next(_endpoints[el][rf].begin(), std::min(K, _endpoints[el][rf].size()));

		std::vector<Endpoint *> epts;
		// 使用std::transform函数，将beg和end之间的每个元素（这里是Endpoint对象）的地址添加到epts向量中
		std::transform(beg, end, std::back_inserter(epts), [](Endpoint &ept)
					   { return &ept; });
		return epts;
	}

	// Function: _worst_endpoints
	std::vector<Endpoint *> Timer::_worst_endpoints(size_t K)
	{

		_update_endpoints();

		std::vector<Endpoint *> epts;
		std::array<std::array<size_t, MAX_TRAN>, MAX_SPLIT> i{{{0, 0}, {0, 0}}};

		// FOR_EACH_EL_RF(el, rf)
		// {
		// 	std::cout << el << " " << rf << std::endl;
		// 	for (auto endpoint : _endpoints[el][rf])
		// 		std::cout << endpoint.slack() << std::endl;
		// }

		for (size_t k = 0; k < K; ++k)
		{

			std::optional<Split> mel;
			std::optional<Tran> mrf;

			/*
			#define FOR_EACH_EL_RF_IF(el, rf, c) \
			  for (auto [el, rf] : SPLIT_TRAN)   \
				if (c)
			*/

			FOR_EACH_EL_RF_IF(el, rf, i[el][rf] < _endpoints[el][rf].size())
			{
				if (!mel || _endpoints[el][rf][i[el][rf]] < _endpoints[*mel][*mrf][i[*mel][*mrf]])
				{
					mel = el;
					mrf = rf;
				}
			}

			if (!mel)
				break;

			epts.push_back(&_endpoints[*mel][*mrf][i[*mel][*mrf]]);
			++i[*mel][*mrf];
		}
		// std::cout << std::endl;
		// for (auto &ept : epts)
		// 	std::cout << ept->slack() << std::endl;

		return epts;
	}

	// Function: _worst_endpoints
	std::vector<Endpoint *> Timer::_worst_endpoints(size_t K, Split el)
	{

		_update_endpoints();

		std::vector<Endpoint *> epts;
		std::array<size_t, MAX_TRAN> i{0, 0};

		for (size_t k = 0; k < K; ++k)
		{

			std::optional<Tran> mrf;

			FOR_EACH_RF_IF(rf, i[rf] < _endpoints[el][rf].size())
			{
				if (!mrf || _endpoints[el][rf][i[rf]] < _endpoints[el][*mrf][i[*mrf]])
				{
					mrf = rf;
				}
			}

			if (!mrf)
				break;

			epts.push_back(&_endpoints[el][*mrf][i[*mrf]]);
			++i[*mrf];
		}

		return epts;
	}

	// Function: _worst_endpoints
	std::vector<Endpoint *> Timer::_worst_endpoints(size_t K, Tran rf)
	{

		_update_endpoints();

		std::vector<Endpoint *> epts;
		std::array<size_t, MAX_SPLIT> i{0, 0};

		for (size_t k = 0; k < K; ++k)
		{

			std::optional<Split> mel;

			FOR_EACH_EL_IF(el, i[el] < _endpoints[el][rf].size())
			{
				if (!mel || _endpoints[el][rf][i[el]] < _endpoints[*mel][rf][i[*mel]])
				{
					mel = el;
				}
			}

			if (!mel)
				break;

			epts.push_back(&_endpoints[*mel][rf][i[*mel]]);
			++i[*mel];
		}

		return epts;
	}

	// TODO (Guannan)
	// Function: _worst_endpoints
	std::vector<Endpoint *> Timer::_worst_endpoints(const PathGuide &guide)
	{

		_update_endpoints();

		std::vector<Endpoint *> epts;

		return epts;
	}

}; // end of namespace ot. -----------------------------------------------------------------------
