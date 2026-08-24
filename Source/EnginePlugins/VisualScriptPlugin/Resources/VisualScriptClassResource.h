/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptClassResource : public xiiScriptClassResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptClassResource, xiiScriptClassResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiVisualScriptClassResource);

public:
  xiiVisualScriptClassResource();
  ~xiiVisualScriptClassResource();

private:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual xiiUniquePtr<xiiScriptInstance> Instantiate(xiiReflectedClass& inout_owner, xiiWorld* pWorld) const override;

  xiiSharedPtr<xiiVisualScriptDataStorage>           m_pConstantDataStorage;
  xiiSharedPtr<const xiiVisualScriptDataDescription> m_pInstanceDataDesc;
  xiiSharedPtr<xiiVisualScriptInstanceDataMapping>   m_pInstanceDataMapping;
};
