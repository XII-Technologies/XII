#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class xiiExposedParametersAttribute;
class xiiExposedParameterCommandAccessor;

/// \brief Default state provider handling variant maps with the xiiExposedParametersAttribute set. Reflects the default value defined in the xiiExposedParameter.
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

private:
  const xiiDocumentObject*             m_pObject              = nullptr;
  const xiiAbstractProperty*           m_pProp                = nullptr;
  const xiiExposedParametersAttribute* m_pAttrib              = nullptr;
  const xiiAbstractProperty*           m_pParameterSourceProp = nullptr;
};
