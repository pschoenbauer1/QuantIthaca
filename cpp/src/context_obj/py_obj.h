#pragma once

#include <context/graph_value.h>

#include <string>

namespace graph
{

class PyValue : public GraphValue
{
public:
    ~PyValue() override = default;

    static std::string name() { return "PyValue"; }
    std::string type_name() const override { return name(); }
};

}  // namespace graph
