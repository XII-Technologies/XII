/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDocument;
class xiiScene2Document;

class xiiSceneDocumentSettingsBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneDocumentSettingsBase, xiiReflectedClass);
};

class xiiPrefabDocumentSettings : public xiiSceneDocumentSettingsBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPrefabDocumentSettings, xiiSceneDocumentSettingsBase);

public:
  xiiDynamicArray<xiiExposedSceneProperty> m_ExposedProperties;
};

class xiiLayerDocumentSettings : public xiiSceneDocumentSettingsBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerDocumentSettings, xiiSceneDocumentSettingsBase);
};

class xiiSceneDocumentRoot : public xiiDocumentRoot
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneDocumentRoot, xiiDocumentRoot);

public:
  xiiSceneDocumentSettingsBase* m_pSettings;
};

class xiiSceneObjectManager : public xiiDocumentObjectManager
{
public:
  xiiSceneObjectManager();
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;

private:
  virtual xiiStatus InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const override;
  virtual xiiStatus InternalCanSelect(const xiiDocumentObject* pObject) const override;
  virtual xiiStatus InternalCanMove(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProperty, const xiiVariant& index) const override;
};
