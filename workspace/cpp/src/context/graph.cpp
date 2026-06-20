
#include "graph.h"

#include <context/graph_value.h>
#include <context_obj/graph_key.h>
#include <context_obj/py_obj.h>
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
    explicit GraphImpl(Graph& graph) : _graph(graph) {}

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

    CPtr<GraphValue> get_value(const GraphKey& key) const
    {
        utils::ReadLock lock(_mutex);

        const auto iter = _map.find(key);
        if (iter == _map.end())
        {
            THROW << "Key " << to_string(key) << " not found.";
        }
        return iter->second->value();
    }
    bool contains(const GraphKey& key) const
    {
        utils::ReadLock lock(_mutex);
        return __contains(key);
    }

    bool is_empty(const GraphKey& key) const
    {
        utils::ReadLock lock(_mutex);
        const auto iter = _map.find(key);
        if (iter == _map.end())
        {
            THROW << "Key " << to_string(key) << " not found.";
        }
        return iter->second->is_empty();
    }

    bool empty() const
    {
        utils::ReadLock lock(_mutex);
        return _map.empty();
    }

    KeySet keys() const
    {
        utils::ReadLock lock(_mutex);
        KeySet result;
        for (const auto& key : std::views::keys(_map))
        {
            result.insert(key);
        }
        return result;
    }

    void set_value(const GraphKey& key, const CPtr<GraphValue>& value)
    {
        utils::ReadLock lock(_mutex);
        __set_value(key, value);
    };

    void compute()
    {
        utils::WriteLock lock2(_compute_mutex);

        {
            utils::WriteLock lock(_mutex);
            __forward_closure();
        }

        batch_compute_python_leaf_nodes(_graph.get());

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
                for (const auto& dependent : _dependents[key])
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

graph::Graph::Graph() : _impl(std::make_shared<GraphImpl>(*this)) {}

CPtr<graph::GraphValue> graph::Graph::get_value(const GraphKey& key) const
{
    return _impl->get_value(key);
}

void graph::Graph::insert(const GraphKey& key, CPtr<GraphValue> value)
{
    _impl->init(key);
    set_value(key, value);
}

void graph::Graph::insert(const GraphKey& key)
{
    _impl->init(key);
}
void graph::Graph::insert(const KeySet& key)
{
    _impl->init(key);
}

void graph::Graph::set_value(const GraphKey& key, CPtr<GraphValue> value)
{
    if (key.allows_py_value_subclasses())
    {
        PS_EXPECT_THAT(std::dynamic_pointer_cast<const PyValue>(value))
            << "Error in 'Graph::set_value': Key " << key.name() << " requires "
            << key.value_type_name() << ".";
    }
    else
    {
        PS_EXPECT_THAT(value->type_name() == key.value_type_name())
            << "Error in 'Graph::set_value': Value Type " << value->type_name() << ", but Key "
            << key.name() << " requires " << key.value_type_name() << ".";
    }
    _impl->set_value(key, value);
}

void graph::Graph::compute()
{
    _impl->compute();
}

bool graph::Graph::empty() const
{
    return _impl->empty();
}

graph::KeySet graph::Graph::keys() const
{
    return _impl->keys();
}

bool graph::Graph::contains(const GraphKey& key) const
{
    return _impl->contains(key);
}

bool graph::Graph::is_empty(const GraphKey& key) const
{
    return _impl->is_empty(key);
}

void graph::Graph::GraphImpl::__init(const GraphKey& key)
{
    if (__contains(key))
    {
        THROW << "Key " << to_string(key) << " already set.";
    }
    _map[key] = std::make_shared<ValueWrapper>();
    _nodes[key] = key.make_builder();
};

namespace graph
{
namespace
{
PyBuilderFactoryFn g_py_builder_factory;
PyBatchComputeLeafNodesFn g_py_batch_compute_leaf_nodes;
}  // namespace

void register_py_builder_factory(PyBuilderFactoryFn fn)
{
    g_py_builder_factory = std::move(fn);
}

CPtr<GraphBuilder> make_py_builder(const std::string& value_type_name, const GraphKey& key)
{
    if (!g_py_builder_factory)
    {
        THROW << "Python builder factory is not registered for " << value_type_name << ".";
    }
    return g_py_builder_factory(value_type_name, key);
}

void register_py_batch_compute_leaf_nodes(PyBatchComputeLeafNodesFn fn)
{
    g_py_batch_compute_leaf_nodes = std::move(fn);
}

void batch_compute_python_leaf_nodes(Graph& graph)
{
    if (g_py_batch_compute_leaf_nodes)
    {
        g_py_batch_compute_leaf_nodes(graph);
    }
}

}  // namespace graph
