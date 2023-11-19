#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptInstance : public xiiScriptInstance
{
public:
  xiiVisualScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld, const xiiSharedPtr<xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pInstanceDataDesc, const xiiSharedPtr<xiiVisualScriptInstanceDataMapping>& pInstanceDataMapping);

  virtual void ApplyParameters(const xiiArrayMap<xiiHashedString, xiiVariant>& parameters) override;

  xiiVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  xiiVisualScriptDataStorage* GetInstanceDataStorage() { return m_pInstanceDataStorage.Borrow(); }

private:
  xiiSharedPtr<xiiVisualScriptDataStorage>         m_pConstantDataStorage;
  xiiUniquePtr<xiiVisualScriptDataStorage>         m_pInstanceDataStorage;
  xiiSharedPtr<xiiVisualScriptInstanceDataMapping> m_pInstanceDataMapping;
};
