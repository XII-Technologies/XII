/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>

using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

struct xiiShaderResourceDescriptor
{
};

class XII_GRAPHICSCORE_DLL xiiShaderResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiShaderResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiShaderResource, xiiShaderResourceDescriptor);

public:
  xiiShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  xiiArrayPtr<const xiiHashedString> GetUsedPermutationVariables() const { return m_PermutationVariablesUsed; }
  xiiArrayPtr<const xiiGALPermutationVariable> GetFixedPermutationVariables() const { return m_FixedPermutationVariables; }

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiHybridArray<xiiHashedString, 16> m_PermutationVariablesUsed;
  xiiHybridArray<xiiGALPermutationVariable, 16> m_FixedPermutationVariables;
  bool                                m_bShaderResourceIsValid;
};
