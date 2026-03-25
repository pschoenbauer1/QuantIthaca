param(
  [ValidateSet("msvc", "clang")]
  [string]$Compiler = "msvc",
  [ValidateSet("Debug", "Release")]
  [string]$Config = "Debug",
  [string]$PythonVersion = "3.12",
  [switch]$RunTests
)

$ErrorActionPreference = "Stop"

function Get-VsDevCmdPath {
  if ($env:QUANTITHACA_VSDEVCMD -and (Test-Path -LiteralPath $env:QUANTITHACA_VSDEVCMD)) {
    return $env:QUANTITHACA_VSDEVCMD
  }

  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $install = & $vswhere -latest -products * `
      -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
      -property installationPath 2>$null
    if ($install) {
      $candidate = Join-Path $install "Common7\Tools\VsDevCmd.bat"
      if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
  }

  foreach ($rel in @(
      "Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat",
      "Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat",
      "Microsoft Visual Studio\18\Enterprise\Common7\Tools\VsDevCmd.bat",
      "Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
      "Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
      "Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    )) {
    $candidate = Join-Path $env:ProgramFiles $rel
    if (Test-Path -LiteralPath $candidate) { return $candidate }
  }
  return $null
}

$preset = if ($Compiler -eq "msvc") {
  if ($Config -eq "Debug") { "windows-msvc-debug" } else { "windows-msvc-release" }
} else {
  if ($Config -eq "Debug") { "windows-clang-debug" } else { "windows-clang-release" }
}

# MSVC and clang-cl need the Visual Studio environment (Ninja + compilers on PATH).
$vsDevCmd = Get-VsDevCmdPath
if (-not $vsDevCmd) {
  throw @"
Could not find VsDevCmd.bat (Visual Studio C++ tools).
Install the Desktop development with C++ workload, or set QUANTITHACA_VSDEVCMD to the full path of VsDevCmd.bat.
"@
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$repoCmd = $repoRoot

$steps = @(
  "cd /d `"$repoCmd`"",
  "uv sync --python $PythonVersion --extra test",
  "uv run --python $PythonVersion cmake --preset $preset",
  "uv run --python $PythonVersion cmake --build --preset $preset"
)
if ($RunTests) {
  $steps += "uv run --python $PythonVersion ctest --preset $preset --output-on-failure"
  $steps += "uv run --python $PythonVersion pytest"
}
$afterVs = $steps -join " && "

$cmdLine = "call `"$vsDevCmd`" -arch=amd64 -host_arch=amd64 && $afterVs"
Write-Host "Using VsDevCmd: $vsDevCmd" -ForegroundColor DarkGray
& cmd.exe /c $cmdLine
exit $LASTEXITCODE
