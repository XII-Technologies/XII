# Native NuGet dependencies

XII's CMake utilities acquire exact native NuGet packages, import reviewed asset mappings, stage runtime dependencies, and export relocatable SDK targets. The implementation lives in `Source/BuildSystem/CMake/CMakeUtils/xiiUtilsNuGet.cmake`, which is included through `xiiUtils.cmake`. Package acquisition requires CMake and an HTTPS connection or a prefilled cache. It runs no NuGet client, MSBuild package files, installation scripts, or language runtimes.

## Persistent package cache

`xii_nuget_init()` defines the following cache options.

| Variable | Default | Meaning |
| --- | --- | --- |
| `XII_NUGET_ROOT` | `${XII_ROOT}/Workspace/Nuget` | Persistent cache shared by build directories and configurations |
| `XII_NUGET_SOURCE` | `https://api.nuget.org/v3-flatcontainer` | HTTPS flat-container feed, or a `file://` flat-container mirror |
| `XII_NUGET_OFFLINE` | `OFF` | Reject downloads and use cached verified archives |

Package IDs are case-insensitive. Numeric version components and casing are normalized, and build metadata is excluded from package identity. Versions remain explicit. There is no configure-time latest-version lookup or upgrade.

```text
Workspace/Nuget/
└── microsoft.native.quic.msquic.openssl/
    └── 2.6.4/
        ├── microsoft.native.quic.msquic.openssl.2.6.4.nupkg
        ├── Extracted/
        ├── .extracted
        └── .lock
```

A version lock serializes acquisition, lookup, validation and publication across CMake processes. Downloads first go to `.nupkg.download`. The SHA-256 pin must match before the archive is renamed. Extraction first goes to `.extracting`. Raw ZIP local and central filenames, libarchive entry names and types, package identity, required OPC metadata, and the extracted-file inventory are checked before publication. Traversal paths, Windows device names, alternate streams, ambiguous names, links, and case collisions fail acquisition. ZIP64, split archives, encrypted archives and special entries require a separate reviewed adapter.

`.extracted` records the normalized identity, archive hash, and SHA-256 of every extracted file. Reuse validates the archive and the complete extracted inventory. A missing or modified extraction is rebuilt from its valid cached archive, including offline. An invalid cached archive fails with its path for diagnosis and explicit removal. Interrupted staging is discarded on the next acquisition. Removing a CMake build directory preserves the cache. A custom cache root must be absolute and outside the build directory.

## Fetch and find packages

```cmake
xii_nuget_fetch_package(
  PACKAGE Microsoft.Native.Quic.MsQuic.OpenSSL
  VERSION 2.6.4
  SHA256 370baf73845ac516a7acb53394ebf889ca947f7826a0e74f8f8c332b3627adb5
  OUT_PACKAGE_DIR MSQUIC_PACKAGE
)
```

`MSQUIC_PACKAGE` points to the verified `Extracted` directory. An optional `SOURCE` overrides the flat-container feed for that call. The feed layout is `<source>/<normalized-id>/<normalized-version>/<id>.<version>.nupkg`. SHA-256 pins use 64 lowercase hexadecimal digits.

`xii_nuget_find_package` accepts the same identity, hash, dependencies and output arguments and returns an empty path for a cache miss. It never downloads or repairs the cache. `xii_nuget_validate_package` instead takes `OUT_VALID` and reports whether a complete cached package passes validation. Acquisition and metadata failures include a CMake fatal diagnostic.

## Explicit dependency boundaries

Fetch a package's dependencies first, pin each version and checksum, and pass their normalized identities in `DEPENDENCIES`.

```cmake
xii_nuget_fetch_package(
  PACKAGE OpenXR.Headers VERSION 1.0.10.2
  SHA256 f1bd28e0eb0cf6c476e211f3e9201dd480d7cf37be4c8ba75ebba9c60b75a8c8
  OUT_PACKAGE_DIR OPENXR_HEADERS
)
xii_nuget_fetch_package(
  PACKAGE OpenXR.Loader VERSION 1.0.10.2
  SHA256 a9286f2977a207689e6838487c85685317309b7bd1f528655e552c760e335b12
  DEPENDENCIES openxr.headers@1.0.10.2
  OUT_PACKAGE_DIR OPENXR_LOADER
)
```

The metadata reader accepts exact dependency versions, including `[version]`. For a bare NuGet minimum version, the selected explicit version must equal that minimum. Unconditional and native dependency groups are supported. Version ranges, managed-framework resolution, framework assembly references and MSBuild-dependent resolution fail with an adapter diagnostic. The utilities never execute `.props`, `.targets` or package scripts. The package adapter explicitly maps assets and supplies any required native compile definitions and link dependencies.

## Import native assets

Every native import declares its platform, architecture, ABI and CRT policy. Asset paths are relative to a package already fetched or found in the current configuration. They must resolve inside that package. PE/COFF, ELF64, thin Mach-O, and native archive member headers are checked against the selected x64 or ARM64 target. Architecture selection uses the target toolchain and generator platform, including cross-compilation.

```cmake
xii_nuget_import_native_library(
  TARGET xiiNuget::MsQuic
  PACKAGE_DIR "${MSQUIC_PACKAGE}"
  TYPE SHARED
  PLATFORM Windows ARCHITECTURE x64
  ABI C CRT DYNAMIC
  LOCATION build/native/bin/x64/msquic.dll
  IMPLIB build/native/lib/x64/msquic.lib
  SYMBOLS build/native/bin/x64/msquic.pdb
  INCLUDE_DIRS build/native/include
)
target_link_libraries(MyLibrary PRIVATE xiiNuget::MsQuic)
```

Use `TYPE STATIC` for static archives. `PLATFORM` matches `CMAKE_SYSTEM_NAME`. `ABI` is `C`, `MSVC`, `GNU`, `Clang` or `AppleClang`. A C ABI declaration requires the adapter to preserve allocation ownership across the boundary. A C++ ABI declaration additionally requires `COMPILER_VERSION`, checks the compiler family and major version, and requires explicit Windows Debug assets. `CRT` is `NONE`, `STATIC` or `DYNAMIC`. Static Windows libraries and C++ Windows boundaries must match `CMAKE_MSVC_RUNTIME_LIBRARY`. Adapters remain responsible for declaring compatible C++ standard-library and public interface policies.

`LOCATION`, `IMPLIB`, and `SYMBOLS` provide assets shared by Debug, Dev and Shipping. `DEBUG_LOCATION`, `DEV_LOCATION`, `SHIPPING_LOCATION` and the corresponding `IMPLIB` and `SYMBOLS` arguments select configuration-specific assets. A release-built C library can deliberately use one mapping for all three configurations. Release maps to Shipping, RelWithDebInfo maps to Dev, and MinSizeRel maps to Shipping. Import libraries are required for Windows shared libraries. Headers, static archives, import libraries and DLLs remain separate assets.

`LINK_LIBRARIES` describes explicit native dependencies and system libraries. Declare imported dependency targets before their consumers. Put per-configuration choices on those dependency targets. `COMPILE_DEFINITIONS` supplies reviewed native interface definitions. Imported targets expose the selected include directories and link requirements through standard CMake target properties.

## Runtime staging and SDK export

```cmake
xii_nuget_stage_runtime(TARGET MyApplication LIBRARIES MyLibrary)
xii_nuget_export_targets(
  FILE "${SDK_OUTPUT}/xiiNuGetTargets.cmake"
  TARGETS xiiNuget::MsQuic
  INSTALL_DESTINATION lib/cmake/XII
)
```

Staging walks explicit link dependencies, including static wrappers and private runtime metadata, and copies the current configuration's shared libraries and available symbols beside the consumer. XII-created targets schedule this operation after their directory finishes linking targets. Each native runtime is staged once per consumer. The destination comes from the consumer target's output directory.

SDK export copies selected headers, libraries and symbols under a `Nuget` asset directory beside the generated target file. The file uses its own location to resolve assets and includes reachable NuGet dependencies. Its optional installation destination installs this self-contained native dependency export. The normal XII export process writes `xiiNuGetTargets.cmake` beside `xiiExport.cmake`, and SDK import loads native targets before the engine targets. Private runtime target metadata survives the engine export, including a shared Foundation boundary. External applications use the same runtime staging helper. A relocated SDK needs its exported native assets and no package cache or package feed.

## Windows networking packages

The official NuGet indexes were checked on November 16, 2026. The following pins are the latest stable packages used by these adapters.

| Package | Version | SHA-256 |
| --- | --- | --- |
| [Microsoft.Native.Quic.MsQuic.OpenSSL](https://www.nuget.org/packages/Microsoft.Native.Quic.MsQuic.OpenSSL/2.6.4) | 2.6.4 | `370baf73845ac516a7acb53394ebf889ca947f7826a0e74f8f8c332b3627adb5` |
| [openssl-native](https://www.nuget.org/packages/openssl-native/3.5.9) | 3.5.9 | `5884c788d624b8a9d9d0510d13f02a9ce487fc0bb570418b8d53444205423d42` |
| [OpenXR.Headers](https://www.nuget.org/packages/OpenXR.Headers/1.0.10.2) | 1.0.10.2 | `f1bd28e0eb0cf6c476e211f3e9201dd480d7cf37be4c8ba75ebba9c60b75a8c8` |
| [OpenXR.Loader](https://www.nuget.org/packages/OpenXR.Loader/1.0.10.2) | 1.0.10.2 | `a9286f2977a207689e6838487c85685317309b7bd1f528655e552c760e335b12` |
| [Microsoft.Holographic.Remoting.OpenXr](https://www.nuget.org/packages/Microsoft.Holographic.Remoting.OpenXr/2.9.4) | 2.9.4 | `0efbc12a5b75fe83d6104d153aa3d80f2cacdfecb90bdbc1d76ebb18d22a791c` |

Windows networking imports MsQuic from `build/native/{bin,lib}/<x64|arm64>` and headers from `build/native/include`. OpenSSL imports `lib/win-<arch>/native/{libssl,libcrypto}.lib`, DLLs from `runtimes/win-<arch>/native`, and headers from `include`. The package has no OpenSSL PDB assets. MsQuic PDBs are staged when available. The official MsQuic DLL includes its own OpenSSL TLS implementation. Foundation continues to use the OpenSSL 3.5.9 DLLs for its TLS record engine and certificate validation, exchanging portable certificate bytes with MsQuic. These packages have no declared NuGet dependencies. The existing OpenXR adapter also uses the shared verified cache and declares the loader's header dependency.

Linux and macOS networking retain their existing pinned source-provider build. The Windows branch returns before locating Python or Perl or defining provider compilation commands. Debug, Dev and Shipping consume the same release C ABI provider assets with explicit mappings.

## Focused validation

Run `python -m unittest discover -s Utilities/CI/tests -p test_nuget.py -v`. The local fixtures test version normalization, default cache placement, reuse between build directories and configurations, offline behavior, hash failures, interrupted HTTPS transfer, stale extraction recovery, concurrent access, unsafe paths and links, metadata, dependency declarations, architecture mismatch, static and shared assets, runtime symbols, installation and SDK relocation. Python and an optional OpenSSL test-certificate CLI are fixture tools, separate from package acquisition.

Windows CI additionally runs `Utilities/CI/validate_network_sdk.py` after the native suites in every configuration. It validates a relocated shared Foundation SDK consumer, reuses the same NuGet cache for an offline static Foundation build, runs only the additional networking tests, and validates the relocated static SDK consumer. Linux Clang Debug remains the default local engine build and networking test configuration.
