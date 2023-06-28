#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

struct xiiVisualScriptDataDescription;
class xiiVisualScriptDataStorage;

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptClassResource : public xiiScriptClassResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptClassResource, xiiScriptClassResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiVisualScriptClassResource);

public:
  xiiVisualScriptClassResource();
  ~xiiVisualScriptClassResource();

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual xiiUniquePtr<xiiScriptInstance> Instantiate(xiiReflectedClass& owner, xiiWorld* pWorld) const override;

  xiiSharedPtr<const xiiVisualScriptDataStorage>     m_pConstantDataStorage;
  xiiSharedPtr<const xiiVisualScriptDataDescription> m_pVariableDataDesc;
};
