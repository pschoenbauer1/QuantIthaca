#pragma once

#include <pybind11/embed.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

#include "graph_py_factory.h"

namespace py = pybind11;

namespace py_runtime
{

inline std::filesystem::path repo_root_from(const std::filesystem::path& source_file)
{
    return source_file.parent_path().parent_path().parent_path().parent_path();
}

inline void ensure_python_runtime(const std::filesystem::path& repo_root)
{
    const auto python_dir = (repo_root / "python").string();
    const auto site_packages = (repo_root / ".venv" / "Lib" / "site-packages").string();
    const auto pyvenv_cfg = repo_root / ".venv" / "pyvenv.cfg";

    static std::once_flag interpreter_once;
    std::call_once(interpreter_once,
                   [&]()
                   {
                       std::ifstream cfg(pyvenv_cfg);
                       std::string line;
                       while (std::getline(cfg, line))
                       {
                           const std::string prefix = "home = ";
                           if (line.rfind(prefix, 0) == 0)
                           {
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
    std::call_once(sys_path_once,
                   [&]()
                   {
                       py::gil_scoped_acquire gil{};
                       py::module_ sys = py::module_::import("sys");
                       py::list sys_path = sys.attr("path");
                       sys_path.attr("insert")(0, site_packages);
                       sys_path.attr("insert")(0, python_dir);
                   });
}

inline void ensure_python_runtime()
{
    ensure_python_runtime(repo_root_from(std::filesystem::path(__FILE__)));
}

inline void ensure_core_bind(const std::filesystem::path& repo_root)
{
    static std::once_flag once;
    std::call_once(once,
                   [&]()
                   {
                       ensure_python_runtime(repo_root);

                       const auto build_dir = repo_root / "build";
                       if (std::filesystem::exists(build_dir))
                       {
                           py::gil_scoped_acquire gil{};
                           py::module_ sys = py::module_::import("sys");
                           py::list sys_path = sys.attr("path");

#ifdef _WIN32
                           constexpr const char* k_ext = ".pyd";
#else
                           constexpr const char* k_ext = ".so";
#endif
                           for (const auto& entry :
                                std::filesystem::recursive_directory_iterator(build_dir))
                           {
                               const auto& path = entry.path();
                               const auto filename = path.filename().string();
                               if (filename.starts_with("core_bind") && path.extension() == k_ext)
                               {
                                   sys_path.attr("insert")(0, path.parent_path().string());
                                   break;
                               }
                           }
                       }

                       py::gil_scoped_acquire gil{};
                       py::module_::import("core_bind");
                       graph::install_py_builder_factory();
                       graph::install_py_batch_compute();
                   });
}

inline void ensure_core_bind()
{
    ensure_core_bind(repo_root_from(std::filesystem::path(__FILE__)));
}

}  // namespace py_runtime
