
#include "graph.h"

#include <ranges>

#include "containers.h"
#include "graph_node.h"
#include "graph_value.h"
#include "threads.h"

// ------------------------- Graph -------------------------

class graph::Graph::GraphImpl {
    KeyMap<Ptr<ValueWrapper>> _map;
    KeyMap<CPtr<Node>> _nodes;
    KeyMap<KeySet> _dependents;

    std::reference_wrapper<Graph> _graph;

    utils::Mutex _mutex;

   public:
    GraphImpl(Graph& graph,
              std::unordered_map<GraphKey, Ptr<ValueWrapper>, utils::TupleHash>&& initial_map)
        : _graph(graph), _map(std::move(initial_map)) {}

    CPtr<GraphValue> value(const GraphKey& key) const {
        utils::ReadLock(_mutex);

        const auto iter = _map.find(key);
        if (iter == _map.end()) {
            THROW << "Key " << to_string(key) << " not found.";
        }
        return iter->second->value();
    }
    bool contains(const GraphKey& key) const {
        utils::ReadLock(_mutex);
        return _map.find(key) != _map.end();
    }

    void init(const GraphKey& key);

    void insert(const GraphKey& key, const CPtr<GraphValue>& value) {
        init(key);
        utils::WriteLock(_mutex);
        _map.at(key)->set_value(value);
    };
    void set_value(const GraphKey& key, const CPtr<GraphValue>& value) {
        if (!contains(key)) {
            THROW << "Key " << to_string(key) << " does not exist.";
        }

        utils::WriteLock(_mutex);
        _map.at(key)->set_value(value);
    };

    void forward_closure() {
        std::vector<GraphKey> stack(std::views::keys(_map).begin(), std::views::keys(_map).end());

        while (!stack.empty()) {
            const auto key = stack.back();
            stack.pop_back();
            for (const auto& dependency_key : _nodes.at(key)->dependencies()) {
                _dependents[dependency_key].insert(key);
                if (!contains(dependency_key)) {
                    init(dependency_key);
                    stack.push_back(dependency_key);
                }
            }
        }
    }

    template <KeyLike Key>
    CPtr<Node> create_node(const Key& key) const;

    void request(const KeySet& keys) {
        for (const auto& key : keys) {
            init(key);
        }
    }

    void compute() {
        forward_closure();

        KeyMap<int> unmet_dependencies{};
        std::vector<GraphKey> ready;
        for (const auto& [key, node] : _nodes) {
            unmet_dependencies[key] = 0;
            for (const auto& dependency : node->dependencies()) {
                if (_map.at(dependency)->is_empty()) {
                    ++unmet_dependencies[key];
                }
            }
            if (unmet_dependencies[key] == 0) {
                ready.push_back(key);
            }
        }
        while (!ready.empty()) {
            const auto key = ready.back();
            ready.pop_back();
            const auto iter = _map.find(key);
            PS_EXPECT_THAT(iter != _map.end());
            if (iter->second->is_empty()) {
                try {
                    iter->second->set_value(_nodes.at(key)->value(_graph));
                } catch (...) {
                    iter->second->set_error(std::current_exception());
                }
                for (const auto& dependent : _dependents.at(key)) {
                    unmet_dependencies[dependent]--;
                    if (unmet_dependencies[dependent] == 0) {
                        ready.push_back(dependent);
                    }
                }
            }
            // Post Condtion check
            for (const auto& [key, value] : _map) {
                PS_EXPECT_THAT(value->is_empty())
                    << "Value for " << to_string(key)
                    << " was not computed. This is likely due to cyclical dependencies.";
            }
        }
    }
};

graph::Graph::Graph()
    : _impl(std::make_shared<GraphImpl>(
          *this, std::unordered_map<GraphKey, Ptr<ValueWrapper>, utils::TupleHash>{})) {}

graph::Graph::Graph(std::unordered_map<GraphKey, Ptr<ValueWrapper>, utils::TupleHash> initial_map)
    : _impl(std::make_shared<GraphImpl>(*this, std::move(initial_map))) {}

CPtr<graph::GraphValue> graph::Graph::value(const GraphKey& key) const {
    return _impl->value(key);
}

void graph::Graph::__insert(const GraphKey& key, const CPtr<GraphValue>& value) {
    return _impl->insert(key, value);
}

void graph::Graph::request(const std::unordered_set<GraphKey>& key) {}

// ------------------------- Nodes -------------------------

template <>
CPtr<graph::Node> graph::Graph::GraphImpl::create_node(const DummyKey1& key) const {
    return std::make_shared<DummyNode1>(key);
}

template <>
CPtr<graph::Node> graph::Graph::GraphImpl::create_node(const DummyKey2& key) const {
    return std::make_shared<DummyNode2>(key);
}

void graph::Graph::GraphImpl::init(const GraphKey& key) {
    if (contains(key)) {
        THROW << "Key " << to_string(key) << " already set.";
    }
    utils::WriteLock(_mutex);
    _map[key] = std::make_shared<ValueWrapper>();
    _nodes[key] = std::visit([this](const auto& key_) { return this->create_node(key_); }, key);
};

// ------------------------- Dummy Nodees -------------------------

graph::DummyNode1::DummyNode1(const DummyKey1& key) : _key(key) {
    _dependencies.insert(GraphKey(DummyKey2{.str = "key1dep"}));
}

graph::DummyNode2::DummyNode2(const DummyKey2& key) : _key(key) {}