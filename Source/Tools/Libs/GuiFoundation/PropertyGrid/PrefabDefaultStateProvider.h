/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// Default state provider that reflects the default state defined in the prefab template.
class XII_GUIFOUNDATION_DLL xiiPrefabDefaultStateProvider : public xiiDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  xiiPrefabDefaultStateProvider(const xiiUuid& rootObjectGuid, const xiiUuid& createFromPrefab, const xiiUuid& prefabSeedGuid, xiiInt32 iRootDepth);
  virtual xiiInt32        GetRootDepth() const override;
  virtual xiiColorGammaUB GetBackgroundColor() const override;
  virtual xiiString       GetStateProviderName() const override { return "Prefab"; }

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;

private:
  const xiiUuid m_RootObjectGuid;
  const xiiUuid m_CreateFromPrefab;
  const xiiUuid m_PrefabSeedGuid;
  xiiInt32      m_iRootDepth = 0;
};
