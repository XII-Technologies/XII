#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// \brief This describes the shader creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderCreationDescription : public xiiHashableStruct<xiiGALShaderCreationDescription>
{
  xiiGALShaderCreationDescription();
  ~xiiGALShaderCreationDescription();

  bool HasValidByteCode() const;

  xiiBitflags<xiiGALShaderType>             m_ShaderType = xiiGALShaderType::Unknown; ///< The shader type. The default is xiiGALShaderType::Unknown.
  xiiScopedRefPointer<xiiGALShaderByteCode> m_ByteCode;                               ///< The shader byte code. See xiiGALShaderByteCode.

  bool operator==(const xiiGALShaderCreationDescription& rhs) const;
};

/// \brief Interface that defines methods to manipulate a shader object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShader : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShader, xiiGALDeviceObject);

public:
  /// \brief This returns the creation description for this object.
  XII_NODISCARD const xiiGALShaderCreationDescription& GetDescription() const;

  /// \brief This returns the vertex input layout of the vertex shader.
  xiiArrayPtr<const xiiGALVertexInputLayout> GetVertexInputLayout() const;

protected:
  friend class xiiGALDevice;

  xiiGALShader(xiiGALDevice* pDevice, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShader();

  virtual xiiResult InitPlatform() = 0;

  virtual xiiResult DeInitPlatform() = 0;

protected:
  xiiGALShaderCreationDescription m_Description;
};


#include <GraphicsFoundation/Shader/Implementation/Shader_inl.h>
