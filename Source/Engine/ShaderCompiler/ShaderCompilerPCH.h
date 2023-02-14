#pragma once

#include <ShaderCompiler/ShaderCompiler.h>

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

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
