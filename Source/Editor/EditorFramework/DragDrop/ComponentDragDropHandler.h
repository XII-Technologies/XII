#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class xiiDocument;
class xiiDragDropInfo;

class XII_EDITORFRAMEWORK_DLL xiiComponentDragDropHandler : public xiiAssetDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiComponentDragDropHandler, xiiAssetDragDropHandler);

protected:
  void CreateDropObject(const xiiVec3& vPosition, const char* szType, const char* szProperty, const xiiVariant& value, xiiUuid parent, xiiInt32 iInsertChildIndex);

  void AttachComponentToObject(const char* szType, const char* szProperty, const xiiVariant& value, xiiUuid ObjectGuid);

  void MoveObjectToPosition(const xiiUuid& guid, const xiiVec3& vPosition, const xiiQuat& qRotation);

  void MoveDraggedObjectsToPosition(xiiVec3 vPosition, bool bAllowSnap, const xiiVec3& normal);

  void SelectCreatedObjects();

  void BeginTemporaryCommands();

  void EndTemporaryCommands();

  void CancelTemporaryCommands();

  xiiDocument*                m_pDocument;
  xiiHybridArray<xiiUuid, 16> m_DraggedObjects;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;

  virtual void OnDragUpdate(const xiiDragDropInfo* pInfo) override;

  virtual void OnDragCancel() override;

  virtual void OnDrop(const xiiDragDropInfo* pInfo) override;

  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  xiiVec3 m_vAlignAxisWithNormal = xiiVec3::ZeroVector();
};
