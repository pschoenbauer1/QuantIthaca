param(
  [string[]]$PythonVersions = @("3.11", "3.12", "3.13")
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "wheelhouse" | Out-Null

foreach ($py in $PythonVersions) {
  uv python install $py
  uv build --wheel --python $py --out-dir wheelhouse
}
