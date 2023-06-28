#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

class xiiComponent;
class xiiTypeScriptBinding;

class XII_TYPESCRIPTPLUGIN_DLL xiiTypeScriptInstance : public xiiScriptInstance
{
public:
  xiiTypeScriptInstance(xiiComponent& owner, xiiTypeScriptBinding& binding);

  virtual void ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters) override;

  xiiTypeScriptBinding& GetBinding() { return m_Binding; }

  xiiComponent& GetComponent() { return m_Component; }

private:
  xiiTypeScriptBinding& m_Binding;
  xiiComponent&         m_Component;
};

class XII_TYPESCRIPTPLUGIN_DLL xiiTypeScriptClassResource : public xiiScriptClassResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptClassResource, xiiScriptClassResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTypeScriptClassResource);

public:
  xiiTypeScriptClassResource();
  ~xiiTypeScriptClassResource();

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual xiiUniquePtr<xiiScriptInstance> Instantiate(xiiReflectedClass& owner, xiiWorld* pWorld) const override;

private:
  xiiUuid m_Guid;
};
