#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class xiiManipulatorAttribute;
class xiiDocumentObject;
struct xiiDocumentObjectPropertyEvent;
struct xiiQtDocumentWindowEvent;
class xiiObjectAccessorBase;
class xiiGridSettingsMsgToEngine;

class XII_EDITORFRAMEWORK_DLL xiiManipulatorAdapter
{
public:
  xiiManipulatorAdapter();
  virtual ~xiiManipulatorAdapter();

  void SetManipulator(const xiiManipulatorAttribute* pAttribute, const xiiDocumentObject* pObject);

  virtual void QueryGridSettings(xiiGridSettingsMsgToEngine& out_gridSettings) {}

private:
  void DocumentObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e);

protected:
  virtual xiiTransform       GetOffsetTransform() const;
  virtual xiiTransform       GetObjectTransform() const;
  xiiObjectAccessorBase*     GetObjectAccessor() const;
  const xiiAbstractProperty* GetProperty(const char* szProperty) const;

  virtual void Finalize()             = 0;
  virtual void Update()               = 0;
  virtual void UpdateGizmoTransform() = 0;

  void BeginTemporaryInteraction();
  void EndTemporaryInteraction();
  void CancelTemporayInteraction();
  void ChangeProperties(const char* szProperty1, xiiVariant value1, const char* szProperty2 = nullptr, xiiVariant value2 = xiiVariant(), const char* szProperty3 = nullptr, xiiVariant value3 = xiiVariant(), const char* szProperty4 = nullptr, xiiVariant value4 = xiiVariant(), const char* szProperty5 = nullptr, xiiVariant value5 = xiiVariant(), const char* szProperty6 = nullptr, xiiVariant value6 = xiiVariant());

  bool                           m_bManipulatorIsVisible;
  const xiiManipulatorAttribute* m_pManipulatorAttr;
  const xiiDocumentObject*       m_pObject;

  void ClampProperty(const char* szProperty, xiiVariant& value) const;
};
