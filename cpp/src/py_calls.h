#pragma once

#include <pybind11/embed.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace py = pybind11;

namespace callback {

inline std::string test_callback() {
    try {
        const auto repo_root =
            std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
        const auto python_dir = (repo_root / "python").string();
        const auto site_packages = (repo_root / ".venv" / "Lib" / "site-packages").string();
        const auto pyvenv_cfg = repo_root / ".venv" / "pyvenv.cfg";

        static std::once_flag py_init_once;
        std::call_once(py_init_once, [&]() {
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

        py::gil_scoped_acquire gil{};
        py::module_ sys = py::module_::import("sys");
        py::list sys_path = sys.attr("path");
        sys_path.attr("insert")(0, site_packages);
        sys_path.attr("insert")(0, python_dir);

        py::module_ mod = py::module_::import("cpp_callbacks");
        py::object func = mod.attr("test_callback");
        py::object result = func();
        return result.cast<std::string>();
    } catch (const py::error_already_set& e) {
        return std::string("Python error: ") + e.what();
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

}  // namespace callback
