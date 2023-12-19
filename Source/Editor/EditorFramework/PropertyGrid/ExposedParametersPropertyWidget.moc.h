#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class QToolButton;
class QAction;

class XII_EDITORFRAMEWORK_DLL xiiExposedParameterCommandAccessor : public xiiObjectProxyAccessor
{
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
  const xiiExposedParameter*  GetExposedParam(const xiiDocumentObject* pObject, const char* szParamName);
  const xiiRTTI*              GetExposedParamsType(const xiiDocumentObject* pObject);
  const xiiRTTI*              GetCommonExposedParamsType(const xiiHybridArray<xiiPropertySelection, 8>& items);
  bool                        IsExposedProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

public:
  const xiiAbstractProperty* m_pParameterProp       = nullptr;
  const xiiAbstractProperty* m_pParameterSourceProp = nullptr;
};

class XII_EDITORFRAMEWORK_DLL xiiQtExposedParameterPropertyWidget : public xiiQtVariantPropertyWidget
{
  Q_OBJECT;

protected:
  virtual void InternalSetValue(const xiiVariant& value);
};

class XII_EDITORFRAMEWORK_DLL xiiQtExposedParametersPropertyWidget : public xiiQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT

public:
  xiiQtExposedParametersPropertyWidget();
  virtual ~xiiQtExposedParametersPropertyWidget();
  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  virtual void                 OnInit() override;
  virtual xiiQtPropertyWidget* CreateWidget(xiiUInt32 index) override;
  virtual void                 UpdateElement(xiiUInt32 index) override;
  virtual void                 UpdatePropertyMetaState() override;

private:
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void FlushQueuedChanges();
  bool RemoveUnusedKeys(bool bTestOnly);
  bool FixKeyTypes(bool bTestOnly);
  void UpdateActionState();

private:
  xiiUniquePtr<xiiExposedParameterCommandAccessor> m_pProxy;
  xiiObjectAccessorBase*                           m_pSourceObjectAccessor = nullptr;
  xiiString                                        m_sExposedParamProperty;
  mutable xiiDynamicArray<xiiExposedParameter>     m_Parameters;
  bool                                             m_bNeedsUpdate         = false;
  bool                                             m_bNeedsMetaDataUpdate = false;

  QToolButton* m_pFixMeButton        = nullptr;
  QAction*     m_pRemoveUnusedAction = nullptr;
  QAction*     m_pFixTypesAction     = nullptr;
};
