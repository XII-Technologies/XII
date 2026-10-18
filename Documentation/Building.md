# Building and testing XII

## Scope and prerequisites

XII uses CMake, C++23, and native platform toolchains. The build configurations are
`Debug`, `Dev`, and `Shipping`. Use CMake 3.22+ for the presets; VS 2026 requires a
CMake version that recognizes the `Visual Studio 18 2026` generator. Install Git
and Python 3.11+ for the infrastructure utilities. Keep several GB free for a CPU
build and substantially more for full engine builds, symbols, Qt, and SDK downloads.

```sh
git clone --recurse-submodules https://github.com/XII-Technologies/XII.git
cd XII
git submodule update --init --recursive
cmake --list-presets
```

`Source/ThirdParty/SDL` and `Data/Tools/Precompiled` are public Git submodules.
The recorded commit, rather than a submodule's latest branch, defines the dependency.
Other third-party code is vendored in the tree. There is no vcpkg/Conan manifest.

## CPU build on Windows

Install Visual Studio 2022 with **Desktop development with C++**, an x64 MSVC
toolset and Windows SDK. The hosted workflow uses `windows-2025` and its VS 2022
installation. The local development tree also has a `vs2026x64` preset.

```powershell
cmake --preset ci-windows
cmake --build Workspace/ci-windows --config Debug --parallel 2 --target FoundationTest CoreTest ToolsFoundationTest
python Utilities/CI/check_test_discovery.py Workspace/ci-windows Debug
ctest --test-dir Workspace/ci-windows -C Debug -L '^headless$' --no-tests=error --output-on-failure --output-junit Workspace/ci-windows/ctest-Debug.xml
cmake --build Workspace/ci-windows --config Shipping --parallel 2 --target FoundationTest CoreTest ToolsFoundationTest
ctest --test-dir Workspace/ci-windows -C Shipping -L '^headless$' --no-tests=error --output-on-failure
```

For a machine with VS 2026 instead, use a fresh build directory and override the
generator: `cmake --preset ci-windows -G "Visual Studio 18 2026"`. Do not switch
generators in an already configured directory. Compiler outputs remain isolated
under `Workspace/ci-windows`, including configuration-specific Bin/Lib directories.

## CPU build on Linux

Hosted jobs target Ubuntu 24.04 x64, GCC 14, and scheduled Clang 18 with libstdc++.
Install only the dependencies needed for this scope:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends cmake ninja-build gcc-14 g++-14 clang-18 lld-18 ccache pkg-config uuid-dev
cmake --preset ci-linux-gcc -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14
cmake --build Workspace/ci-linux-gcc --parallel 2 --target FoundationTest CoreTest ToolsFoundationTest
python3 Utilities/CI/check_test_discovery.py Workspace/ci-linux-gcc Debug
ctest --test-dir Workspace/ci-linux-gcc -C Debug -L '^headless$' --no-tests=error --output-on-failure
```

Use `ci-linux-clang` with `-DCMAKE_C_COMPILER=clang-18
-DCMAKE_CXX_COMPILER=clang++-18` for Clang. Set `-DCMAKE_BUILD_TYPE=Shipping`
at configure time for an optimized single-configuration build. The `--config`
argument alone does not change a Ninja build's configuration.

Optional compiler caching uses `-DCMAKE_C_COMPILER_LAUNCHER=ccache
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache`. CI caches compiler objects only, with
configuration/build-system keys; ccache also checks the compiler and command line.
Generated build trees, libraries, and executables are not restored from a cache.

These presets intentionally disable Qt, SDL, BC7 encoder support, Vulkan, and D3D12.
They build Foundation, Core, ToolsFoundation, their transitive dependencies and
the native test framework. They do not certify a complete engine or editor build.

## Full development builds and tools

Existing `vs2022x64`, `vs2026x64`, and `linux-{gcc,clang}-{debug,dev,shipping}`
presets remain available. Windows CMake can download its configured Qt package;
the current package is Qt 6.11 built for VS 2026's toolset. VS 2022 users enabling
Qt must supply a compatible Qt installation via `XII_QT_DIR`. Linux editor support
is explicitly experimental and requires Qt 6.11 plus window-system dependencies.
Distribution Qt packages may be older. `Setup.sh` is a broad legacy bootstrap,
not the authoritative minimal CI dependency list.

Vulkan builds use bundled headers and a versioned DXC download configured under
`Source/BuildSystem/CMake`. A compatible Vulkan loader, device and driver are
required to run the backend. D3D12 is currently forced OFF in
`xiiUtilsD3D12.cmake`; requesting ON does not enable it.

For a Windows developer installation with the corresponding dependencies:

```powershell
cmake --preset vs2026x64
cmake --build Workspace/vs2026x64 --config Debug --target GraphicsCoreTest GraphicsFoundationTest GpuDrivenScene --parallel 2
```

The regular preset puts executables under `Output/Bin/WinVs2026Debug64`.
`GraphicsCoreTest.exe -nosave -nogui -all` exercises CPU renderer contracts;
`GraphicsFoundationTest.exe -nosave -nogui -all` runs device tests across compiled
backends. Use `-renderer Vulkan` only to explicitly restrict a backend run.
The `GpuDrivenScene.exe -frames 3` sample is a separate runtime check, not a substitute
for tests. Full-editor verification remains separate from these targeted builds.

`ArchiveTool` can be built with the CPU preset as a standalone tool:
`cmake --build Workspace/ci-windows --config Shipping --target ArchiveTool`.
The manual tool workflow creates a candidate artifact for review; it does not
automatically update the PrecompiledTools repository.

## Test classification and results

CTest calls the existing TestFramework executables with `-noSave -all` and disables
interactive HTML/console behavior. It supplies `-sdkRoot` explicitly so generated
`Data/Base` build directories cannot shadow source test assets. Native HTML/JSON diagnostics live in
`Workspace/<preset>/TestResults/<configuration>/<suite>`; CTest records its own logs
under `Testing/Temporary`. Suites run serially because fixtures share state.

- `headless`: FoundationTest, CoreTest, ToolsFoundationTest; GraphicsCoreTest when
  configured in a developer build. Hosted presets expect exactly the first three.
- `hardware`: GraphicsFoundationTest, TextureTest, GameEngineTest, InspectorTest,
  and EditorEngineProcessFrameworkTest as declared by their existing CMake files.
- `interactive`: unclassified/editor GUI suites, including EditorFrameworkTest.

`-nogui` disables the test-runner GUI, not a test's need for a GPU or desktop.
Run hardware and editor suites on a trusted machine with the required drivers,
assets, SDKs and desktop session. No hosted GPU success is claimed. Review each
suite's output for platform-dependent unavailable cases; a green aggregate is not
evidence that every optional device feature executed.

## Quality checks

```sh
python3 Utilities/CI/check_repository.py
yamllint .github
actionlint
python3 Utilities/CI/check_changed_source.py --base <full-base-commit-sha> --formatter <clang-format-21-executable>
```

Use the formatter in the pinned PrecompiledTools submodule: Linux
`clang-format/clang-format_linux_21.0.0`, Windows
`clang-format/clang-format_21.0.0.exe`. The check uses the existing
`Utilities/ClangFormatDiff.py` and root style on changed lines only. Apply a suggested
patch with `git apply Workspace/Quality/clang-format.patch`, then rebuild.
Actionlint 1.7.12 is checksum verified in CI. YAML lint uses `.yamllint.yml`.

MSVC `/analyze` is available through `XII_ENABLE_COMPILER_STATIC_ANALYSIS=ON` and
runs in extended validation. `Utilities/RunClangTidy.ps1` remains available for
specialist local use with its custom naming check, dependency analyzer, and compile
database; it is not a reliable stock-LLVM hosted gate. CodeQL covers first-party C++
and workflow source without requiring the full editor build.

## Troubleshooting

- Missing submodule/tool files: initialize submodules, verify recorded revisions
  and read access. Do not replace recorded revisions with moving branches.
- Wrong configuration or output path: use CTest's target discovery rather than
  guessing executable paths. Multi-config VS builds need `--config`; Ninja needs
  `CMAKE_BUILD_TYPE` when configuring.
- CMake reports Qt missing: use the CPU preset or supply a matching Qt 6.11 install.
- Memory pressure: reduce `--parallel`, and limit nested MSVC compilation with
  `$env:_CL_='/MP2'`. For PDB/toolchain contention, retry MSBuild serially using
  `cmake --build <directory> --config Debug --target <target> -- /m:1 /nr:false`.
- Failures in editor or graphics consumers are not waived by CPU CI. Include the
  first error and complete configure/build/test logs in a build/platform issue.
- macOS requires additional portability work in Texture/DirectXTex and its test
  dependencies. Current presets do not provide a verified macOS engine build.
