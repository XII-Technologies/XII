# XII Engine

XII is a modular C++23 engine for real-time simulation and interactive applications.
It separates platform services, world simulation, resources, rendering, and native
tools into libraries so applications can use the layers they need. The repository
includes a renderer, an editor, developer tools, samples, and a native test framework.

The project is under active development. Individual library tests do not establish
that every editor workflow, sample, or graphics feature is ready for deployment.

## Architecture

| Layer | Implemented responsibilities |
| --- | --- |
| Foundation | Containers, math/SIMD, memory, threading, files, serialization, reflection, logging, and platform abstractions |
| Core | Worlds, objects/components, resources, input, scripting integration, and directed graphs |
| GraphicsFoundation | Graphics device, resource, command, and synchronization interfaces |
| GraphicsCore | Render graphs, serialized pipeline resources, geometry residency, materials, and GPU scene systems |
| GraphicsVulkan | Vulkan implementation and SPIR-V shader compilation |
| Tools and editor | Asset/document infrastructure, Qt editor, Inspector, archive and conversion utilities |

See [rendering documentation](Documentation/Rendering/WorldRenderingMigration.md) and the
[documentation index](Documentation/Readme.md). D3D12 source exists but is currently
forced off by the build system while renderer contracts evolve. Advanced rendering
and editor work should be evaluated against their specific tests and limitations.

## Platform status

| Environment | Status and scope |
| --- | --- |
| Windows x64, MSVC | Primary development platform. Hosted CI targets VS 2022 in Debug and Shipping for CPU modules. Local VS 2026 validation is recorded separately. |
| Linux x64, GCC / Clang | Experimental. Hosted GCC CPU checks and scheduled Clang validation are configured; this does not certify Linux editor or graphics runtime support. |
| macOS, Apple Clang | Platform code exists; no current whole-engine/test validation. Texture/test-framework compatibility and platform dependencies must be resolved before adding a required job. |
| Other architectures | Not validated by this CI matrix. |

The [infrastructure guide](Documentation/Infrastructure.md) records the exact CI
matrix and verification limits. `Shipping` is XII's optimized release configuration;
`Release` is not one of its build configurations. C++23 is required.

## Build and test

Clone with both public submodules at their recorded revisions:

```sh
git clone --recurse-submodules https://github.com/XII-Technologies/XII.git
cd XII
```

For a CPU-only Windows build, install Visual Studio 2022 with Desktop development
with C++, a Windows SDK, CMake 3.22 or newer, Git, and Python 3.11 or newer:

```powershell
cmake --preset ci-windows
cmake --build Workspace/ci-windows --config Debug --parallel 2 --target FoundationTest CoreTest ToolsFoundationTest
ctest --test-dir Workspace/ci-windows -C Debug -L '^headless$' --no-tests=error --output-on-failure
```

For Linux dependencies, toolchain selection, full developer builds, running tools,
and troubleshooting, use the authoritative [build guide](Documentation/Building.md).
The CPU preset needs neither Qt nor a GPU. Full editor builds have additional
requirements, including Qt 6.11, and are outside hosted CPU CI coverage.

## Contribute and evaluate

- [Contribution guide](CONTRIBUTING.md) and [coding standards](Coding-Standards.md)
- [Issue tracker](https://github.com/XII-Technologies/XII/issues) and [support](SUPPORT.md)
- [Security reporting](SECURITY.md)
- [Releases](https://github.com/XII-Technologies/XII/releases) and [release process](Documentation/Infrastructure.md#releases)
- [Project website](https://www.xiitechnologies.com)

## License and dependencies

[LICENSE.md](LICENSE.md) currently states **All Rights Reserved**. Public source
availability is not an open-source license grant. Obtain appropriate permission
before reuse or redistribution. Third-party components retain their own licenses
and notices; see `Source/ThirdParty` and `Source/Engine/Foundation/ThirdParty`.
