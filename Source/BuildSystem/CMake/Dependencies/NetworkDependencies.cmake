# Copyright (c) Theophilus Eriata. All Rights Reserved.

# Private maintained providers, compiled separately so their build flags never change engine targets.
# Runtime libraries are staged beside Foundation and exported through SDK runtime metadata.
function(xii_link_network_providers TARGET_NAME)
  xii_pull_all_vars()
  if(XII_CMAKE_PLATFORM_WINDOWS)
    xii_nuget_fetch_package(PACKAGE Microsoft.Native.Quic.MsQuic.OpenSSL VERSION 2.6.4
      SHA256 370baf73845ac516a7acb53394ebf889ca947f7826a0e74f8f8c332b3627adb5 OUT_PACKAGE_DIR MSQUIC_PACKAGE)
    xii_nuget_fetch_package(PACKAGE openssl-native VERSION 3.5.9
      SHA256 5884c788d624b8a9d9d0510d13f02a9ce487fc0bb570418b8d53444205423d42 OUT_PACKAGE_DIR OPENSSL_PACKAGE)
    if(XII_CMAKE_ARCHITECTURE_ARM AND XII_CMAKE_ARCHITECTURE_64BIT)
      set(PACKAGE_ARCH arm64)
    elseif(XII_CMAKE_ARCHITECTURE_64BIT)
      set(PACKAGE_ARCH x64)
    else()
      message(FATAL_ERROR "Windows networking NuGet providers support x64 and ARM64.")
    endif()
    # These private C ABIs own their allocations. The OpenSSL DLLs use a static CRT, and the
    # official MsQuic OpenSSL DLL includes its own TLS implementation and uses the system UCRT.
    xii_nuget_import_native_library(TARGET xiiNuget::OpenSSLCrypto PACKAGE_DIR "${OPENSSL_PACKAGE}"
      TYPE SHARED PLATFORM Windows ARCHITECTURE "${PACKAGE_ARCH}" ABI C CRT STATIC
      LOCATION "runtimes/win-${PACKAGE_ARCH}/native/libcrypto-3-${PACKAGE_ARCH}.dll"
      IMPLIB "lib/win-${PACKAGE_ARCH}/native/libcrypto.lib" INCLUDE_DIRS include)
    xii_nuget_import_native_library(TARGET xiiNuget::OpenSSL PACKAGE_DIR "${OPENSSL_PACKAGE}"
      TYPE SHARED PLATFORM Windows ARCHITECTURE "${PACKAGE_ARCH}" ABI C CRT STATIC
      LOCATION "runtimes/win-${PACKAGE_ARCH}/native/libssl-3-${PACKAGE_ARCH}.dll"
      IMPLIB "lib/win-${PACKAGE_ARCH}/native/libssl.lib" INCLUDE_DIRS include
      LINK_LIBRARIES xiiNuget::OpenSSLCrypto)
    xii_nuget_import_native_library(TARGET xiiNuget::MsQuic PACKAGE_DIR "${MSQUIC_PACKAGE}"
      TYPE SHARED PLATFORM Windows ARCHITECTURE "${PACKAGE_ARCH}" ABI C CRT DYNAMIC
      LOCATION "build/native/bin/${PACKAGE_ARCH}/msquic.dll" IMPLIB "build/native/lib/${PACKAGE_ARCH}/msquic.lib"
      SYMBOLS "build/native/bin/${PACKAGE_ARCH}/msquic.pdb" INCLUDE_DIRS build/native/include)
    add_custom_target(NetworkProviders)
    target_link_libraries(${TARGET_NAME} PRIVATE xiiNuget::MsQuic xiiNuget::OpenSSL)
    set_property(GLOBAL PROPERTY XII_NETWORK_PROVIDER_INCLUDE "${MSQUIC_PACKAGE}/build/native/include;${OPENSSL_PACKAGE}/include")
    set_property(GLOBAL PROPERTY XII_NETWORK_PROVIDER_LINK xiiNuget::MsQuic xiiNuget::OpenSSL)
    return()
  endif()
  find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
  find_package(Perl REQUIRED)
  set(XII_NETWORK_DEPENDENCY_ARCHIVE_CACHE "${XII_ROOT}/Workspace/shared/NetworkDependencyArchives" CACHE PATH "Verified OpenSSL and MsQuic source archive cache (may be prefilled for offline builds)")
  set(XII_NETWORK_DEPENDENCY_BUILD_JOBS 2 CACHE STRING "Maximum parallel compiler jobs for private networking providers")
  xii_pull_all_vars()
  xii_pull_output_vars("${XII_OUTPUT_DIRECTORY_LIB}" "${XII_OUTPUT_DIRECTORY_DLL}")
  set(PROVIDER_ROOT "${CMAKE_BINARY_DIR}/NetworkDependencies")
  set(PROVIDER_INCLUDE "${PROVIDER_ROOT}/include")
  set(PROVIDER_BIN "$<IF:$<CONFIG:Debug>,${OUTPUT_DLL_DEBUG},$<IF:$<CONFIG:Dev>,${OUTPUT_DLL_DEV},${OUTPUT_DLL_RELEASE}>>")
  set(PROVIDER_LIB "$<IF:$<CONFIG:Debug>,${OUTPUT_LIB_DEBUG},$<IF:$<CONFIG:Dev>,${OUTPUT_LIB_DEV},${OUTPUT_LIB_RELEASE}>>")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Aa][Rr][Cc][Hh]64|[Aa][Rr][Mm]64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
    set(PROVIDER_ARCH arm64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PROVIDER_ARCH x64)
  else()
    message(FATAL_ERROR "The pinned networking provider build supports x64 and arm64 targets.")
  endif()
  if(APPLE)
    set(RUNTIME_NAMES libssl.3.dylib libcrypto.3.dylib libmsquic.2.dylib)
    set(PROVIDER_LINK "${PROVIDER_BIN}/libmsquic.2.dylib" "${PROVIDER_BIN}/libssl.3.dylib" "${PROVIDER_BIN}/libcrypto.3.dylib")
  else()
    set(RUNTIME_NAMES libssl.so.3 libcrypto.so.3 libmsquic.so.2)
    set(PROVIDER_LINK "${PROVIDER_BIN}/libmsquic.so.2" "${PROVIDER_BIN}/libssl.so.3" "${PROVIDER_BIN}/libcrypto.so.3")
  endif()
  set(PROVIDER_OUTPUTS "${PROVIDER_BIN}/xii-network-dependencies.ready")
  foreach(FILE_NAME IN LISTS RUNTIME_NAMES)
    list(APPEND PROVIDER_OUTPUTS "${PROVIDER_BIN}/${FILE_NAME}")
  endforeach()
  if(APPLE)
    list(APPEND PROVIDER_OUTPUTS "${PROVIDER_BIN}/libssl.dylib" "${PROVIDER_BIN}/libcrypto.dylib" "${PROVIDER_BIN}/libmsquic.dylib")
  elseif(UNIX)
    list(APPEND PROVIDER_OUTPUTS "${PROVIDER_BIN}/libssl.so" "${PROVIDER_BIN}/libcrypto.so" "${PROVIDER_BIN}/libmsquic.so")
  endif()
  list(APPEND PROVIDER_OUTPUTS ${PROVIDER_LINK})
  list(REMOVE_DUPLICATES PROVIDER_OUTPUTS)
  add_custom_command(OUTPUT ${PROVIDER_OUTPUTS}
    COMMAND Python3::Interpreter "${XII_ROOT}/Utilities/Build/build_network_dependencies.py"
      --root "${PROVIDER_ROOT}" --cache "${XII_NETWORK_DEPENDENCY_ARCHIVE_CACHE}"
      --bin "${PROVIDER_BIN}" --lib "${PROVIDER_LIB}" --include "${PROVIDER_INCLUDE}"
      --arch "${PROVIDER_ARCH}" --cc "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}"
      --cmake "${CMAKE_COMMAND}" --perl "${PERL_EXECUTABLE}" --vs "${CMAKE_GENERATOR_INSTANCE}"
      --jobs "${XII_NETWORK_DEPENDENCY_BUILD_JOBS}"
    DEPENDS "${XII_ROOT}/Utilities/Build/build_network_dependencies.py"
    COMMENT "Building pinned OpenSSL 3.5.9 and MsQuic 2.6.4 providers" VERBATIM)
  add_custom_target(NetworkProviders DEPENDS ${PROVIDER_OUTPUTS})
  add_dependencies(${TARGET_NAME} NetworkProviders)
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE "${PROVIDER_INCLUDE}")
  target_link_libraries(${TARGET_NAME} PRIVATE ${PROVIDER_LINK})
  if(APPLE)
    set(NETWORK_LOADER_DIRECTORY "@loader_path")
  elseif(UNIX)
    set(NETWORK_LOADER_DIRECTORY "$ORIGIN")
  endif()
  if(NETWORK_LOADER_DIRECTORY)
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY BUILD_RPATH "${NETWORK_LOADER_DIRECTORY}")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY INSTALL_RPATH "${NETWORK_LOADER_DIRECTORY}")
  endif()
  foreach(CONFIG IN ITEMS DEBUG DEV RELEASE)
    set_property(TARGET ${TARGET_NAME} PROPERTY XII_NETWORK_RUNTIME_${CONFIG} "${OUTPUT_DLL_${CONFIG}}")
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES XII_NETWORK_RUNTIME_${CONFIG})
  endforeach()
  set_property(TARGET ${TARGET_NAME} PROPERTY XII_NETWORK_RUNTIME_FILES "${RUNTIME_NAMES}")
  set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES XII_NETWORK_RUNTIME_FILES)
  set_property(GLOBAL PROPERTY XII_NETWORK_PROVIDER_INCLUDE "${PROVIDER_INCLUDE}")
  set_property(GLOBAL PROPERTY XII_NETWORK_PROVIDER_LINK "${PROVIDER_LINK}")
endfunction()
