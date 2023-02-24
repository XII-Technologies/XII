#pragma once

#include <ShaderCompiler/ShaderCompiler.h>

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if D3D12_SUPPORTED || VULKAN_SUPPORTED
#  undef NULL
#  define NULL 0

#  if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#    include "WinHPreface.h"

#    include <Unknwn.h>
#    include <atlbase.h>
#    include <atlcom.h>
#    include <guiddef.h>

#    include "dxc/dxcapi.h"

#    include "WinHPostface.h"

#    include "DXCompiler.hpp"
#  elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#    include "WinHPreface.h"

#    include <Unknwn.h>
#    include <atlbase.h>
#    include <atlcom.h>
#    include <guiddef.h>

#    include "dxc/dxcapi.h"

#    include "WinHPostface.h"

#    include "DXCompiler.hpp"
#  elif XII_ENABLED(XII_PLATFORM_LINUX)
#    include "DXCompiler.hpp"

#    include "dxc/dxcapi.h"
#  else
#    error DXC Shader Compiler is not supported on this platform
#  endif
#endif

/// \brief XII ComPtr to automatically free resources.
template <typename T>
struct xiiComPtr
{
public:
  xiiComPtr() {}
  ~xiiComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  xiiComPtr(const xiiComPtr& other) :
    m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T*       operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** Put()
  {
    XII_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty xiiComPtr");
    return &m_ptr;
  }

  T* RawPtr()
  {
    return m_ptr;
  }

  T** RawDblPtr()
  {
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

////////// Utility Functions //////////

#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <RendererFoundation/Shader/Shader.h>

Diligent::SHADER_TYPE GALToDiligentShaderStage(xiiGALShaderStage::Enum e);
