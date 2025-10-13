// Python bindings for status_rollup library using pybind11

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <optional>
#include <string>

#include "status_rollup/status.hpp"
#include "status_rollup/status_tree.hpp"

namespace py = pybind11;
using namespace status_rollup;

PYBIND11_MODULE(_status_rollup, m) {
    m.doc() = "Python bindings for status_rollup - hierarchical status monitoring and rollup";

    // Status enum
    py::enum_<Status>(m, "Status", py::arithmetic())
        .value("GREEN", Status::Green, "Healthy/Normal status")
        .value("YELLOW", Status::Yellow, "Warning/Degraded status")
        .value("RED", Status::Red, "Critical/Failed status")
        .value("UNKNOWN", Status::Unknown, "Unknown/Uncomputed status")
        .export_values();

    // Status conversion functions
    m.def("string_to_status", &string_to_status,
          py::arg("s"),
          "Convert string to Status enum. Accepts: 'green', 'yellow', 'red', 'unknown' (case-insensitive)");

    m.def("status_to_string", &status_to_string,
          py::arg("status"),
          "Convert Status enum to string representation");

    // StatusTree class
    py::class_<StatusTree>(m, "StatusTree")
        .def(py::init<>(), "Create a new StatusTree")
        .def("load_config", &StatusTree::load_config,
             py::arg("config_file"),
             "Load tree configuration from JSON file")
        .def("set_status", &StatusTree::set_status,
             py::arg("node_name"),
             py::arg("status"),
             "Set the status of a leaf node")
        .def("compute", &StatusTree::compute,
             "Compute all derived node statuses based on rollup rules")
        .def("get_status", &StatusTree::get_status,
             py::arg("node_name"),
             "Get the status of any node (returns None if node doesn't exist)")
        .def("print_statuses", &StatusTree::print_statuses,
             "Print hierarchical tree visualization to stdout");

    // Add version info
    m.attr("__version__") = "0.1.0";
}
