/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;
struct xiiPhantomRttiManagerEvent;

/// Helper accessor to pretend all exposed parameters always have a value defined.
/// The exposed parameters are stored as just a sparse map. Only the elements that are overwritten from their defaults are actually stored in the component.
/// Thus, requesting the value of an exposed parameter that has not been overwritten results in failure.
/// To fix this, this class will automatically return the default value of an exposed parameter.
/// This allows the tooling code to always show every exposed parameter's value independent on whether it was overwritten or remains at the default value.
class XII_EDITORFRAMEWORK_DLL xiiExposedParameterCommandAccessor : public xiiObjectProxyAccessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedParameterCommandAccessor, xiiObjectProxyAccessor);

public:
  xiiExposedParameterCommandAccessor(xiiObjectAccessorBase* pSource, const xiiAbstractProperty* pParameterProp, const xiiAbstractProperty* pM_pParameterSourceProp);

  virtual xiiStatus GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) override;
  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys) override;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) override;

public:
  const xiiExposedParameters* GetExposedParams(const xiiDocumentObject* pObject);
  const xiiExposedParameter*  GetExposedParam(const xiiDocumentObject* pObject, xiiStringView sParamName);
  const xiiRTTI*              GetExposedParamsType(const xiiDocumentObject* pObject);
  const xiiRTTI*              GetCommonExposedParamsType(const xiiHybridArray<xiiPropertySelection, 8>& items);
  bool                        IsExposedProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

public:
  const xiiAbstractProperty* m_pParameterProp       = nullptr;
  const xiiAbstractProperty* m_pParameterSourceProp = nullptr;
};

/// Accessor to pretend the exposed parameters map property is an object of the generated phantom type.
/// This fake type accessor is created by taking the property name and redirecting to the exposed parameter map's element under that name.
/// As long as no code path is looking at the actual type of the object this works with any property widget.
/// An xiiQtTypeWidget constructed with the exposed parameter type and this accessor will produce a normal type widget that looks like the exposed parameter type but redirects all read / writes into the exposed parameter map property.
/// Additionally, this class ensures the value stored in the map is converted to match the property type exactly.
class XII_EDITORFRAMEWORK_DLL xiiExposedParametersAsTypeCommandAccessor : public xiiObjectProxyAccessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExposedParametersAsTypeCommandAccessor, xiiObjectProxyAccessor);

public:
  xiiExposedParametersAsTypeCommandAccessor(xiiExposedParameterCommandAccessor* pSource);
  xiiExposedParameterCommandAccessor* GetSourceAccessor() const { return static_cast<xiiExposedParameterCommandAccessor*>(m_pSource); }

  virtual xiiStatus GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;
  virtual xiiStatus GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) override;
  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys) override;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) override;

protected:
  xiiStatus GetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value);
  xiiStatus SetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiStatus(xiiVariant& subValue)>& func);

  /// Make sure that any property retrieved via this accessor matches the expected property type to make sure no invalid data is passed into one of the property widgets generated under the type widget.
  void PatchPropertyType(xiiVariant& ref_value, const xiiAbstractProperty* pProp);
};

/// Custom widget for properties annotated with the xiiExposedParametersAttribute attribute.
/// Technically exposed parameters are stored as an xiiVariantDictionary but that leaves much to be desired for usability.
/// This class uses xiiExposedParameterCommandAccessor to always show all exposed parameters in the dictionary even if none were overwritten.
/// Additionally, xiiExposedParametersAsTypeCommandAccessor is used to project the exposed parameters into a phantom type widget to make editing exposed parameters indistinguishable from editing a normal type object.
/// A button can be used to switch between the two representations.
class XII_EDITORFRAMEWORK_DLL xiiQtExposedParametersPropertyWidget : public xiiQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  xiiQtExposedParametersPropertyWidget();
  virtual ~xiiQtExposedParametersPropertyWidget();
  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(xiiUInt32 index) override;
  virtual void UpdatePropertyMetaState() override;
  virtual void GetRequiredElements(xiiDynamicArray<xiiVariant>& out_keys) const override;

private:
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);
  void FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate);
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  static bool s_bRawMode;

private:
  xiiUniquePtr<xiiExposedParameterCommandAccessor>        m_pProxy;
  xiiUniquePtr<xiiExposedParametersAsTypeCommandAccessor> m_pTypeProxy;
  xiiObjectAccessorBase*                                  m_pSourceObjectAccessor = nullptr;
  xiiString                                               m_sExposedParamProperty;
  mutable xiiDynamicArray<xiiExposedParameter>            m_Parameters;
  bool                                                    m_bNeedsUpdate         = false;
  bool                                                    m_bNeedsMetaDataUpdate = false;

  xiiQtTypeWidget* m_pTypeWidget          = nullptr;
  QVBoxLayout*     m_pTypeViewLayout      = nullptr;
  QToolButton*     m_pFixMeButton         = nullptr;
  QToolButton*     m_pToggleRawModeButton = nullptr;
  QAction*         m_pRemoveUnusedAction  = nullptr;
  QAction*         m_pFixTypesAction      = nullptr;
};
