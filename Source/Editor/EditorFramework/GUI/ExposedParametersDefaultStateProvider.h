/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class xiiExposedParametersAttribute;
class xiiExposedParameterCommandAccessor;
class xiiExposedParametersAsTypeCommandAccessor;

/// Default state provider handling variant maps with the xiiExposedParametersAttribute set. Reflects the default value defined in the xiiExposedParameter.
class XII_EDITORFRAMEWORK_DLL xiiExposedParametersDefaultStateProvider : public xiiDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);
  xiiExposedParametersDefaultStateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  virtual xiiInt32        GetRootDepth() const override;
  virtual xiiColorGammaUB GetBackgroundColor() const override;
  virtual xiiString       GetStateProviderName() const override { return "Exposed Parameters"; }

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;

  virtual bool      IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;

protected:
  const xiiDocumentObject*             m_pObject              = nullptr;
  const xiiAbstractProperty*           m_pProp                = nullptr;
  const xiiExposedParametersAttribute* m_pAttrib              = nullptr;
  const xiiAbstractProperty*           m_pParameterSourceProp = nullptr;
};

/// Default state provider handling variant maps with the xiiExposedParametersAttribute set that are visualized as their respective phantom type.
/// This class builds on top of xiiExposedParametersDefaultStateProvider and only adds the logic to redirect the phantom type + phantom property requested into the actual underlying variant map of the exposed parameters.
/// The provider is only valid if the target accessor is of type xiiExposedParametersAsTypeCommandAccessor.
class XII_EDITORFRAMEWORK_DLL xiiExposedParametersAsTypeDefaultStateProvider : public xiiExposedParametersDefaultStateProvider
{
public:
  static xiiSharedPtr<xiiDefaultStateProvider> CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  xiiExposedParametersAsTypeDefaultStateProvider(xiiExposedParametersAsTypeCommandAccessor* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  virtual xiiVariant GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff) override;
  virtual bool       IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus  RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;

private:
  xiiResult GetDefaultValueInternal(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiVariant& out_DefaultValue);

private:
  xiiExposedParametersAsTypeCommandAccessor* m_pAccessor = nullptr;
};
