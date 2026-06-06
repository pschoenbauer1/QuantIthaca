# QuantIthaca

Hybrid Python + C++ repository using CMake + Ninja, nanobind bindings, uv-managed environments, and cross-platform build/test automation.

## Requirements

- Python 3.11, 3.12, or 3.13
- `uv`
- CMake >= 3.25
- Ninja
- C++ toolchain:
  - Windows: MSVC and/or clang-cl
  - Linux: GCC and/or Clang

## Local environment (uv)

```bash
uv sync --python 3.12 --extra test
```

## Build locally

### Windows (PowerShell)

```powershell
./scripts/build_windows.ps1 -Compiler msvc -Config Debug
./scripts/build_windows.ps1 -Compiler msvc -Config Release
./scripts/build_windows.ps1 -Compiler clang -Config Debug
./scripts/build_windows.ps1 -Compiler clang -Config Release
```

### Linux

```bash
./scripts/build_linux.sh gcc Debug 3.12
./scripts/build_linux.sh gcc Release 3.12
./scripts/build_linux.sh clang Debug 3.12
./scripts/build_linux.sh clang Release 3.12
```

## Run tests

### Combined C++ + Python

Windows:

```powershell
./scripts/build_windows.ps1 -Compiler msvc -Config Debug -RunTests
```

Linux:

```bash
./scripts/build_linux.sh gcc Debug 3.12 true
```

## Build wheels with uv (local)

Windows:

```powershell
./scripts/build_wheels.ps1
```

Linux:

```bash
./scripts/build_wheels.sh
```

All wheels are written to `wheelhouse/`.

## Reproducible Docker wheel builds

```bash
./scripts/docker_build_wheels.sh
```

This builds wheels in Ubuntu containers for Python 3.11/3.12/3.13 and copies outputs into `wheelhouse/`.

## VS Code / Cursor build

The default shortcut for **Run Build Task** is **Ctrl+Shift+B** (not F7). That runs the default task in `.vscode/tasks.json` (`Build (default)`), which invokes `scripts/build_windows.ps1` (MSVC Debug: `uv sync`, CMake preset configure, then build).

Visual Studio uses **F7** for build; VS Code/Cursor does **not** assign F7 to build unless you add a keybinding. To match Visual Studio, open **Keyboard Shortcuts (JSON)** and add:

```json
{
  "key": "f7",
  "command": "workbench.action.tasks.build"
}
```

If the build task fails, ensure **Visual Studio** (or Build Tools) has the **Desktop development with C++** workload so `VsDevCmd.bat` exists, and that `uv` works from a terminal (the script runs `uv sync` and CMake via `uv run`).
