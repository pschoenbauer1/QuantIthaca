
#include "graph.h"

#include <context/graph_node.h>
#include <context/graph_value.h>
#include <context_obj/dummy_obj1.h>
#include <utils/containers.h>
#include <utils/threads.h>

#include <ranges>

// ------------------------- Graph -------------------------

class graph::Graph::GraphImpl
{
    KeyMap<Ptr<ValueWrapper>> _map;
    KeyMap<CPtr<GraphBuilder>> _nodes;
    KeyMap<KeySet> _dependents;

    std::reference_wrapper<Graph> _graph;

    mutable utils::Mutex _mutex;
    mutable utils::Mutex _compute_mutex;

    void __init(const GraphKey& key);
    bool __contains(const GraphKey& key) const { return _map.find(key) != _map.end(); }

    void __set_value(const GraphKey& key, const CPtr<GraphValue>& value)
    {
        if (!__contains(key))
        {
            THROW << "Key " << to_string(key) << " does not exist.";
        }

        _map.at(key)->set_value(value);
    };
    void __forward_closure()
    {
        std::vector<GraphKey> stack(std::views::keys(_map).begin(), std::views::keys(_map).end());

        while (!stack.empty())
        {
            const auto key = stack.back();
            stack.pop_back();
            for (const auto& dependency_key : _nodes.at(key)->dependencies())
            {
                _dependents[dependency_key].insert(key);
                if (!__contains(dependency_key))
                {
                    __init(dependency_key);
                    stack.push_back(dependency_key);
                }
            }
        }
    }

public:
    GraphImpl(Graph& graph,
              std::unordered_map<GraphKey, Ptr<ValueWrapper>, utils::TupleHash>&& initial_map)
        : _graph(graph), _map(std::move(initial_map))
    {
    }

    void init(const GraphKey& key)
    {
        utils::WriteLock lock2(_compute_mutex);
        utils::WriteLock lock(_mutex);
        __init(key);
    }

    void insert(const GraphKey& key, const CPtr<GraphValue>& value)
    {
        utils::WriteLock lock2(_compute_mutex);
        utils::WriteLock lock(_mutex);
        __init(key);
        _map.at(key)->set_value(value);
    }
    void init(const KeySet& keys)
    {
        utils::WriteLock lock2(_compute_mutex);
        utils::WriteLock lock(_mutex);
        for (const auto& key : keys)
        {
            __init(key);
        }
    }

    CPtr<GraphValue> value(const GraphKey& key) const
    {
        utils::ReadLock lock(_mutex);

        const auto iter = _map.find(key);
        if (iter == _map.end())
        {
            THROW << "Key " << to_string(key) << " not found.";
        }
        return iter->second->value();
    }
    bool contains(const GraphKey& key)
    {
        utils::ReadLock lock(_mutex);
        return __contains(key);
    }

    void set_value(const GraphKey& key, const CPtr<GraphValue>& value)
    {
        utils::ReadLock lock(_mutex);
        __set_value(key, value);
    };

    template <KeyLike Key>
    CPtr<GraphBuilder> create_node(const Key& key) const;

    void compute()
    {
        utils::WriteLock lock2(_compute_mutex);

        {
            utils::WriteLock lock(_mutex);
            __forward_closure();
        }

        KeyMap<int> unmet_dependencies{};
        std::vector<GraphKey> ready;
        for (const auto& [key, node] : _nodes)
        {
            unmet_dependencies[key] = 0;
            for (const auto& dependency : node->dependencies())
            {
                if (_map.at(dependency)->is_empty())
                {
                    ++unmet_dependencies[key];
                }
            }
            if (unmet_dependencies[key] == 0)
            {
                ready.push_back(key);
            }
        }
        while (!ready.empty())
        {
            const auto key = ready.back();
            ready.pop_back();
            const auto iter = _map.find(key);
            PS_EXPECT_THAT(iter != _map.end());
            if (iter->second->is_empty())
            {
                try
                {
                    iter->second->set_value(_nodes.at(key)->value(_graph));
                }
                catch (...)
                {
                    iter->second->set_error(std::current_exception());
                }
                for (const auto& dependent : _dependents.at(key))
                {
                    unmet_dependencies[dependent]--;
                    if (unmet_dependencies[dependent] == 0)
                    {
                        ready.push_back(dependent);
                    }
                }
            }
        }
        // Post Condtion check
        for (const auto& [key, value] : _map)
        {
            PS_EXPECT_THAT(!value->is_empty())
                << "Value for " << to_string(key)
                << " was not computed. This is likely due to cyclical dependencies.";
        }
    }
};

graph::Graph::Graph() : _impl(std::make_shared<GraphImpl>(*this, KeyMap<Ptr<ValueWrapper>>{})) {}

graph::Graph::Graph(KeyMap<Ptr<ValueWrapper>> initial_map)
    : _impl(std::make_shared<GraphImpl>(*this, std::move(initial_map)))
{
}

CPtr<graph::GraphValue> graph::Graph::value(const GraphKey& key) const
{
    return _impl->value(key);
}

void graph::Graph::insert(const GraphKey& key, CPtr<GraphValue> value)
{
    const auto lambda = [&]<typename Key>(const Key& k)
    {
        using value_type = Key::ValueType;
        auto value_ = std::dynamic_pointer_cast<const value_type>(value);
        PS_EXPECT_THAT(!!value_) << "Error in 'Graph::insert': Value Type " << value->type_name()
                                 << ", but Key " << Key::name() << " requires "
                                 << value_type::name();
    };
    std::visit(lambda, key);
    return _impl->insert(key, value);
}

void graph::Graph::insert(const GraphKey& key)
{
    _impl->init(key);
}
void graph::Graph::insert(const KeySet& key)
{
    _impl->init(key);
}

// ------------------------- GraphBuilders -------------------------

template <>
CPtr<graph::GraphBuilder> graph::Graph::GraphImpl::create_node(const DummyKey1& key) const
{
    return std::make_shared<DummyGraphBuilder1>(key);
}

template <>
CPtr<graph::GraphBuilder> graph::Graph::GraphImpl::create_node(const DummyKey2& key) const
{
    return std::make_shared<DummyGraphBuilder2>(key);
}

void graph::Graph::GraphImpl::__init(const GraphKey& key)
{
    if (__contains(key))
    {
        THROW << "Key " << to_string(key) << " already set.";
    }
    _map[key] = std::make_shared<ValueWrapper>();
    _nodes[key] = std::visit([this](const auto& key_) { return this->create_node(key_); }, key);
};

// ------------------------- Dummy GraphBuilderes -------------------------

graph::DummyGraphBuilder2::DummyGraphBuilder2(const DummyKey2& key) : _key(key) {}