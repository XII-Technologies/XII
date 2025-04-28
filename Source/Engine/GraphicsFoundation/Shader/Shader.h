#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// \brief This describes the shader creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderCreationDescription : public xiiHashableStruct<xiiGALShaderCreationDescription>
{
  xiiGALShaderCreationDescription() :
    xiiHashableStruct()
  {
  }

  ~xiiGALShaderCreationDescription()
  {
    xiiGALShaderByteCode* pByteCode = m_ByteCode;
    m_ByteCode                      = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      XII_DEFAULT_DELETE(pByteCode);
    }
  }

  XII_ALWAYS_INLINE bool HasValidByteCode() const
  {
    if (m_ShaderType == xiiGALShaderType::Unknown)
      return false;

    return m_ByteCode != nullptr && m_ByteCode->IsValid();
  }

  xiiBitflags<xiiGALShaderType>             m_ShaderType = xiiGALShaderType::Unknown; ///< The shader type. The default is xiiGALShaderType::Unknown.
  xiiScopedRefPointer<xiiGALShaderByteCode> m_ByteCode;                               ///< The shader byte code. See xiiGALShaderByteCode.
};

/// \brief Interface that defines methods to manipulate a shader object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShader : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShader, xiiGALDeviceObject);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALShaderCreationDescription& GetDescription() const { return m_Description; }

  /// \brief This returns the vertex input layout of the vertex shader.
  XII_ALWAYS_INLINE xiiArrayPtr<const xiiGALVertexInputLayout> GetVertexInputLayout() const
  {
    if (m_Description.m_ShaderType == xiiGALShaderType::Vertex)
    {
      return m_Description.m_ByteCode->m_VertexInputLayout;
    }
    return {};
  }

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALShader(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShader();

  virtual xiiResult InitPlatform() = 0;

  virtual xiiResult DeInitPlatform() = 0;

protected:
  xiiGALShaderCreationDescription m_Description;
};
