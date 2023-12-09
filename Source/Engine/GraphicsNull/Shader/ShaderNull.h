#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSNULL_DLL xiiGALShaderNull final : public xiiGALShader
{
public:
  /// \brief This returns the total number of shader resources.
  virtual xiiUInt32 GetResourceCount() const override final;

  /// \brief This returns a pointer to the array of shader resources.
  virtual void GetResourceDescription(xiiUInt32 uiIndex, xiiGALShaderResourceDescription& out_ResourceDescription) const override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALShaderNull(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/Shader/Implementation/ShaderNull_inl.h>
