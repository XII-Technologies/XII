
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class XII_RENDERERFOUNDATION_DLL xiiGALShaderByteCode : public xiiRefCounted
{
public:
  xiiGALShaderByteCode();

  xiiGALShaderByteCode(const xiiArrayPtr<const xiiUInt8>& pByteCode);

  inline const void* GetByteCode() const;

  inline xiiUInt32 GetSize() const;

  inline bool IsValid() const;

protected:
  void CopyFrom(const xiiArrayPtr<const xiiUInt8>& pByteCode);

  xiiDynamicArray<xiiUInt8> m_Source;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
