#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSNULL_DLL xiiGALShaderNull final : public xiiGALShader
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderNull, xiiGALShader);

public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALShaderNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderNull();

  virtual xiiResult InitPlatform() override final;

  virtual xiiInternal::NewInstance<xiiGALInputLayout> CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description) override;
};
