/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class xiiVariantSubAccessor;

// Default value provider for xiiVariantSubAccessor.
class XII_GUIFOUNDATION_DLL xiiVariantSubDefaultStateProvider : public xiiDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  xiiVariantSubDefaultStateProvider(xiiVariantSubAccessor* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  virtual xiiInt32        GetRootDepth() const override;
  virtual xiiColorGammaUB GetBackgroundColor() const override;
  virtual xiiString       GetStateProviderName() const override { return "Variant"; }

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;
  virtual bool       IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;

private:
  xiiResult GetDefaultValueInternal(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiVariant& out_DefaultValue);

private:
  xiiVariantSubAccessor*     m_pAccessor     = nullptr;
  const xiiDocumentObject*   m_pObject       = nullptr;
  const xiiAbstractProperty* m_pProp         = nullptr;
  xiiObjectAccessorBase*     m_pRootAccessor = nullptr;
};
