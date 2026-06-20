
#include "dummy_obj.h"

#include <context/graph.h>


namespace graph
{

namespace
{
constexpr double k_price_markup = 1.05;
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKey1& key)
{
    return std::make_shared<DummyGraphBuilder1>(key);
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKey2& key)
{
    return std::make_shared<DummyGraphBuilder2>(key);
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKey3& key)
{
    return std::make_shared<DummyGraphBuilder3>(key);
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKey4& key)
{
    return std::make_shared<DummyGraphBuilder4>(key);
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKey5& key)
{
    return std::make_shared<DummyGraphBuilder5>(key);
}

template <>
CPtr<GraphBuilder> make_builder(const DummyKeyPy& key)
{
    return make_py_builder(DummyKeyPy::value_type_name(), GraphKey(key));
}

DummyGraphBuilder1::DummyGraphBuilder1(const DummyKey1& key) : _key(key) {}

DummyGraphBuilder2::DummyGraphBuilder2(const DummyKey2& key) : _key(key) {}

DummyGraphBuilder3::DummyGraphBuilder3(const DummyKey3& key) : _key(key)
{
    _dependencies.insert(GraphKey(DummyKey1{.symbol = key.symbol}));
}

DummyGraphBuilder4::DummyGraphBuilder4(const DummyKey4& key) : _key(key)
{
    _dependencies.insert(GraphKey(DummyKey3{.symbol = key.symbol}));
    _dependencies.insert(GraphKey(DummyKey2{.index = key.index}));
}

DummyGraphBuilder5::DummyGraphBuilder5(const DummyKey5& key) : _key(key)
{
    _dependencies.insert(GraphKey(DummyKey4{.symbol = key.symbol, .index = key.index}));
    _dependencies.insert(GraphKey(DummyKey3{.symbol = key.symbol}));
    _dependencies.insert(GraphKey(DummyKey2{.index = key.index}));
}

CPtr<GraphValue> DummyGraphBuilder3::value(const Graph& graph) const
{
    const auto raw = graph.get_value(DummyKey1{.symbol = _key.symbol});
    return std::make_shared<DummyValue3>(raw->price() * k_price_markup);
}

CPtr<GraphValue> DummyGraphBuilder4::value(const Graph& graph) const
{
    const auto adjusted = graph.get_value(DummyKey3{.symbol = _key.symbol});
    const auto benchmark = graph.get_value(DummyKey2{.index = _key.index});
    return std::make_shared<DummyValue4>(adjusted->adjusted_price() - benchmark->rate());
}

CPtr<GraphValue> DummyGraphBuilder5::value(const Graph& graph) const
{
    const auto spread = graph.get_value(DummyKey4{.symbol = _key.symbol, .index = _key.index});
    const auto adjusted = graph.get_value(DummyKey3{.symbol = _key.symbol});
    const auto benchmark = graph.get_value(DummyKey2{.index = _key.index});
    return std::make_shared<DummyValue5>(spread->spread() * adjusted->adjusted_price() /
                                         benchmark->rate());
}

}  // namespace graph
