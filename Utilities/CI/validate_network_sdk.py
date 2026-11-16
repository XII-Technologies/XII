#!/usr/bin/env python3
# Copyright (c) Theophilus Eriata. All Rights Reserved.

"""Build a standalone Foundation networking consumer from a relocated SDK without its NuGet cache."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


def run(arguments, *, cwd):
    print("+ " + " ".join(map(str, arguments)), flush=True)
    subprocess.run(list(map(str, arguments)), cwd=cwd, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--configuration", choices=("Debug", "Dev", "Shipping"), required=True)
    parser.add_argument("--static", action="store_true", help="Configure and validate an independent static Foundation build")
    args = parser.parse_args()
    root = args.root.resolve()
    build = args.build.resolve()
    config = args.configuration
    if args.static:
        build = root / "Workspace" / f"nuget-static-{config}"
        run(["cmake", "--preset", "ci-windows", "-B", build,
             "-DXII_COMPILE_ENGINE_AS_DLL=OFF", "-DXII_NUGET_OFFLINE=ON",
             f"-DXII_OUTPUT_DIRECTORY_DLL={build.as_posix()}/Bin",
             f"-DXII_OUTPUT_DIRECTORY_LIB={build.as_posix()}/Lib"], cwd=root)
        run(["cmake", "--build", build, "--config", config, "--parallel", "2", "--target", "FoundationTest"], cwd=root)
        # Restrict this additional run to Foundation networking tests.
        candidates = list((build / "Bin").glob("*/FoundationTest.exe"))
        candidates = [p for p in candidates if config in p.parent.name]
        if len(candidates) != 1:
            raise RuntimeError(f"Expected one static FoundationTest for {config}, found {candidates}")
        run([candidates[0], "-noGui", "-run", "-close", "-noSave", "-all", "-filter", "Network",
             "-console", "true", "-html", "false", "-assert", "0", "-sdkRoot", root,
             "-outputDir", build / "NetworkTestResults"], cwd=root)

    workspace = root / "Workspace" / f"nuget-sdk-{config}-{'static' if args.static else 'shared'}"
    if workspace.exists():
        shutil.rmtree(workspace)
    original = workspace / "original-sdk"
    original.mkdir(parents=True)
    shutil.copyfile(root / "xiiCMakeConfig.cmake", original / "xiiCMakeConfig.cmake")
    shutil.copyfile(root / "version.txt", original / "version.txt")
    shutil.copytree(root / "Source/BuildSystem/CMake", original / "Source/BuildSystem/CMake")
    shutil.copytree(root / "Source/Engine/Foundation", original / "Source/Engine/Foundation",
                    ignore=shutil.ignore_patterns("*.cpp", "*.c", "CMakeLists.txt", "finalize.cmake"))
    (original / "Source/ThirdParty").mkdir(parents=True, exist_ok=True)
    shutil.copytree(build / "Bin", original / "Output/Bin")
    shutil.copytree(build / "Lib", original / "Output/Lib")
    sdk = workspace / "relocated-sdk"
    original.rename(sdk)
    project = workspace / "consumer"
    project.mkdir()
    (project / "main.cpp").write_text('''#include <Foundation/Configuration/Startup.h>
#include <Foundation/Network/NetworkHost.h>

int main()
{
  xiiStartup::StartupCoreSystems();
  int result = 1;
  {
    xiiNetworkHost host;
    xiiNetworkHostConfiguration configuration;
    configuration.m_Transport = xiiNetworkTransportType::Tcp;
    configuration.m_Authentication = xiiNetworkAuthenticationPolicy::AnonymousDevelopment;
    configuration.m_bAllowInsecureDevelopment = true;
    result = host.Start(configuration).Succeeded() ? 0 : 1;
    host.Shutdown();
  }
  xiiStartup::ShutdownCoreSystems();
  return result;
}
''', encoding="utf-8")
    (project / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.20)
project(NetworkSdkConsumer LANGUAGES C CXX)
set(XII_SDK_DIR "" CACHE PATH "Relocated SDK")
set(XII_ROOT "${XII_SDK_DIR}")
set_property(GLOBAL PROPERTY XII_ROOT "${XII_ROOT}")
include("${XII_SDK_DIR}/xiiCMakeConfig.cmake")
get_property(XII_CMAKE_RELPATH GLOBAL PROPERTY XII_CMAKE_RELPATH)
list(APPEND CMAKE_MODULE_PATH "${XII_SDK_DIR}/${XII_CMAKE_RELPATH}")
include(xiiUtils)
include(xiiPlatform)
set(XII_OUTPUT_DIRECTORY_DLL "${XII_SDK_DIR}/Output/Bin")
set(XII_OUTPUT_DIRECTORY_LIB "${XII_SDK_DIR}/Output/Lib")
xii_include_xiiExport()
xii_configure_external_project()
add_executable(NetworkSdkConsumer main.cpp)
set_target_properties(NetworkSdkConsumer PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)
target_link_libraries(NetworkSdkConsumer PRIVATE Foundation)
xii_stage_external_runtime(NetworkSdkConsumer)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/program-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:NetworkSdkConsumer>")
''', encoding="utf-8")
    consumer_build = workspace / "consumer-build"
    cache = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if not line.startswith(("#", "//")) and ":" in line and "=" in line:
            key, value = line.split("=", 1)
            cache[key.split(":", 1)[0]] = value
    configure = ["cmake", "-S", project, "-B", consumer_build, "-G", cache["CMAKE_GENERATOR"],
                 f"-DXII_SDK_DIR={sdk.as_posix()}", "-DXII_ENABLE_QT_SUPPORT=OFF", "-DXII_NUGET_OFFLINE=ON",
                 f"-DCMAKE_BUILD_TYPE={config}"]
    if cache.get("CMAKE_GENERATOR_PLATFORM"):
        configure += ["-A", cache["CMAKE_GENERATOR_PLATFORM"]]
    if cache.get("CMAKE_GENERATOR_TOOLSET"):
        configure += ["-T", cache["CMAKE_GENERATOR_TOOLSET"]]
    for language in ("C", "CXX"):
        if cache.get(f"CMAKE_{language}_COMPILER"):
            configure += [f"-DCMAKE_{language}_COMPILER={cache[f'CMAKE_{language}_COMPILER']}"]
    run(configure, cwd=root)
    run(["cmake", "--build", consumer_build, "--config", config, "--parallel", "2"], cwd=root)
    executable = Path((consumer_build / f"program-{config}.txt").read_text())
    for name in ("msquic.dll", "libssl-3-x64.dll", "libcrypto-3-x64.dll"):
        if sys.platform == "win32" and not (executable.parent / name).is_file():
            raise RuntimeError(f"NuGet runtime was not staged beside consumer: {name}")
    run([executable], cwd=executable.parent)
    if (sdk / "Workspace/Nuget").exists():
        raise RuntimeError("Relocated SDK unexpectedly depends on a package cache")
    print(f"Relocated {'static' if args.static else 'shared'} Foundation SDK consumer passed in {config}.", flush=True)


if __name__ == "__main__":
    main()
