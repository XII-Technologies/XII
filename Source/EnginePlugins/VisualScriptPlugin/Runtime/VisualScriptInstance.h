#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class XII_VISUALSCRIPTPLUGIN_DLL xiiVisualScriptInstance : public xiiScriptInstance
{
public:
  xiiVisualScriptInstance(xiiReflectedClass& inout_owner, xiiWorld* pWorld, const xiiSharedPtr<xiiVisualScriptDataStorage>& pConstantDataStorage, const xiiSharedPtr<const xiiVisualScriptDataDescription>& pInstanceDataDesc, const xiiSharedPtr<xiiVisualScriptInstanceDataMapping>& pInstanceDataMapping);

  virtual void       SetInstanceVariable(const xiiHashedString& sName, const xiiVariant& value) override;
  virtual xiiVariant GetInstanceVariable(const xiiHashedString& sName) override;

  xiiVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  xiiVisualScriptDataStorage* GetInstanceDataStorage() { return &m_InstanceDataStorage; }

private:
  xiiSharedPtr<xiiVisualScriptDataStorage>         m_pConstantDataStorage;
  xiiSharedPtr<xiiVisualScriptInstanceDataMapping> m_pInstanceDataMapping;

  xiiVisualScriptDataStorage m_InstanceDataStorage;
};
