#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Object.h>

/// \brief This class wraps shader byte code storage.
///
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderByteCode : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderByteCode, xiiGALObject);

public:
  xiiGALShaderByteCode();

  xiiGALShaderByteCode(const xiiArrayPtr<const xiiUInt8>& pByteCode);

  /// \brief This returns a raw pointer to the shader bytecode.
  XII_ALWAYS_INLINE const void* GetByteCode() const;

  /// \brief This returns the size of the shader bytecode.
  XII_ALWAYS_INLINE xiiUInt32 GetSize() const;

  /// \brief This returns true if the shader bytecode is not empty, else returns false.
  XII_ALWAYS_INLINE bool IsValid() const;

protected:
  void CopyFrom(const xiiArrayPtr<const xiiUInt8>& pByteCode);

  xiiDynamicArray<xiiUInt8> m_Source;
};

#include <GraphicsFoundation/Shader/Implementation/ShaderByteCode_inl.h>
