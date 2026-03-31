#pragma once

#include <pybind11/embed.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace py = pybind11;

namespace py_calls {

/// One-time embedded Python setup: env from `.venv/pyvenv.cfg`, interpreter, and `sys.path`.
/// Call at the start of any function that imports or calls into Python (then acquire GIL).
/// Use the no-arg overload unless you need a nonstandard repo layout.
inline void ensure_python_runtime(const std::filesystem::path& repo_root) {
    const auto python_dir = (repo_root / "python").string();
    const auto site_packages = (repo_root / ".venv" / "Lib" / "site-packages").string();
    const auto pyvenv_cfg = repo_root / ".venv" / "pyvenv.cfg";

    static std::once_flag interpreter_once;
    std::call_once(interpreter_once, [&]() {
        std::ifstream cfg(pyvenv_cfg);
        std::string line;
        while (std::getline(cfg, line)) {
            const std::string prefix = "home = ";
            if (line.rfind(prefix, 0) == 0) {
                const std::string home = line.substr(prefix.size());
#ifdef _WIN32
                _putenv_s("PYTHONHOME", home.c_str());
                _putenv_s("PYTHONPATH", (python_dir + ";" + site_packages).c_str());
#else
                setenv("PYTHONHOME", home.c_str(), 1);
                setenv("PYTHONPATH", (python_dir + ":" + site_packages).c_str(), 1);
#endif
                break;
            }
        }
        static py::scoped_interpreter guard{};
    });

    static std::once_flag sys_path_once;
    std::call_once(sys_path_once, [&]() {
        py::gil_scoped_acquire gil{};
        py::module_ sys = py::module_::import("sys");
        py::list sys_path = sys.attr("path");
        sys_path.attr("insert")(0, site_packages);
        sys_path.attr("insert")(0, python_dir);
    });
}

/// Repo root = three parents of `__FILE__` (works for TUs under `cpp/src` or `tests/cpp`).
inline void ensure_python_runtime() {
    ensure_python_runtime(
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path());
}

template <typename ReturnT, typename... Args>
inline ReturnT py_call(const std::string& py_function, Args&&... args) {
    try {
        ensure_python_runtime();

        py::gil_scoped_acquire gil{};
        py::module_ mod = py::module_::import("cpp_callbacks");
        py::object func = mod.attr(py_function.c_str());
        py::object result = func(std::forward<Args>(args)...);
        return result.cast<ReturnT>();
    } catch (const py::error_already_set& e) {
        throw std::runtime_error(std::string("Python error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error: ") + e.what());
    }
}

}  // namespace py_calls
