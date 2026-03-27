#include "graph.h"

#include "graph_value.h"
#include "threads.h"

// ------------------------- Graph -------------------------

class graph::Graph::GraphImpl {
    KeyValueMap _map;
    utils::Mutex _mutex;

   public:
    GraphImpl(KeyValueMap&& initial_map) : _map(std::move(initial_map)) {}

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

    void set_value(const GraphKey& key, const CPtr<GraphValue>& value) {
        if (contains(key)) {
            THROW << "Key " << to_string(key) << " already set.";
        }
        utils::WriteLock(_mutex);
        _map[key] = std::make_shared<ValueWrapper>(value);
    };
};

graph::Graph::Graph() : _impl(std::make_shared<GraphImpl>(KeyValueMap{})) {}

graph::Graph::Graph(KeyValueMap initial_map)
    : _impl(std::make_shared<GraphImpl>(std::move(initial_map))) {}

CPtr<graph::GraphValue> graph::Graph::value(const GraphKey& key) const {
    return _impl->value(key);
}

void graph::Graph::__set_value(const GraphKey& key, const CPtr<GraphValue>& value) {
    return _impl->set_value(key, value);
}


