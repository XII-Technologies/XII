/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// This is the fall back default state provider which handles the default state set via the xiiDefaultAttribute on the reflected type.
class XII_GUIFOUNDATION_DLL xiiAttributeDefaultStateProvider : public xiiDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  virtual xiiInt32        GetRootDepth() const override;
  virtual xiiColorGammaUB GetBackgroundColor() const override;
  virtual xiiString       GetStateProviderName() const override { return "Attribute"; }

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;
};
