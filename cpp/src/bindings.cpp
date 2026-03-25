#include <nanobind/nanobind.h>

#include "core.hpp"

namespace nb = nanobind;

NB_MODULE(core_bind, m) {
  m.doc() = "QuantIthaca native module";
  m.def("add", &qi::add, "Add two integers");
}
