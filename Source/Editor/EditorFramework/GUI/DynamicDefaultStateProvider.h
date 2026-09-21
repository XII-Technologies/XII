/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class xiiDynamicDefaultValueAttribute;
class xiiPropertyPath;

/// Retrieves the dynamic default state of an object or container attributed with xiiDynamicDefaultValueAttribute from an asset's meta data.
class XII_EDITORFRAMEWORK_DLL xiiDynamicDefaultStateProvider : public xiiDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  xiiDynamicDefaultStateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiDocumentObject* pClassObject, const xiiDocumentObject* pRootObject, const xiiAbstractProperty* pRootProp, xiiInt32 iRootDepth);

  virtual xiiInt32        GetRootDepth() const override;
  virtual xiiColorGammaUB GetBackgroundColor() const override;
  virtual xiiString       GetStateProviderName() const override { return "Dynamic"; }

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;

private:
  const xiiReflectedClass* GetMetaInfo(xiiObjectAccessorBase* pAccessor) const;
  const xiiResult          CreatePath(xiiObjectAccessorBase* pAccessor, const xiiReflectedClass* pMeta, xiiPropertyPath& propertyPath, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());

  const xiiDocumentObject*               m_pObject          = nullptr;
  const xiiDocumentObject*               m_pClassObject     = nullptr;
  const xiiDocumentObject*               m_pRootObject      = nullptr;
  const xiiAbstractProperty*             m_pRootProp        = nullptr;
  xiiInt32                               m_iRootDepth       = 0;
  const xiiDynamicDefaultValueAttribute* m_pAttrib          = nullptr;
  const xiiAbstractProperty*             m_pClassSourceProp = nullptr;
  const xiiRTTI*                         m_pClassType       = nullptr;
  const xiiAbstractProperty*             m_pClassProperty   = nullptr;
};
