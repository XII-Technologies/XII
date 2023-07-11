#pragma once

#include <Core/Scripting/DuktapeFunction.h>
#include <Foundation/Memory/CommonAllocators.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;

using duk_context    = duk_hthread;
using duk_c_function = xiiInt32 (*)(duk_context* ctx);

class XII_CORE_DLL xiiDuktapeContext : public xiiDuktapeHelper
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDuktapeContext);

public:
  xiiDuktapeContext(xiiStringView sWrapperName);
  ~xiiDuktapeContext();

  /// \name Basics
  ///@{

  /// \brief Enables support for loading modules via the 'require' function
  void EnableModuleSupport(duk_c_function moduleSearchFunction);

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void  FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void  DukFree(void* pUserData, void* pPointer);

protected:
  bool m_bInitializedModuleSupport = false;

private:
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation, xiiMemoryTrackingFlags::RegisterAllocator | xiiMemoryTrackingFlags::EnableAllocationTracking> m_Allocator;
#  else
  xiiAllocator<xiiMemoryPolicies::xiiHeapAllocation, xiiMemoryTrackingFlags::None> m_Allocator;
#  endif
};

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
