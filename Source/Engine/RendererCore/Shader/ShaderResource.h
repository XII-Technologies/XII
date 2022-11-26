#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

struct xiiShaderResourceDescriptor
{
};

class XII_RENDERERCORE_DLL xiiShaderResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiShaderResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiShaderResource, xiiShaderResourceDescriptor);

public:
  xiiShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  xiiArrayPtr<const xiiHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiHybridArray<xiiHashedString, 16> m_PermutationVarsUsed;
  bool                                m_bShaderResourceIsValid;
};
