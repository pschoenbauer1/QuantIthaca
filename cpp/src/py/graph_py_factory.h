#pragma once

namespace graph
{

// Register the default Python GraphBuilder factory for this module (exe or pyd).
void install_py_builder_factory();

// Register the batch Python leaf-node compute hook (one C++ -> Python call per compute()).
void install_py_batch_compute();

}  // namespace graph
