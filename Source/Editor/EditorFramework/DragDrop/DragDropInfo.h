/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>

class QMimeData;
class QDataStream;
class xiiDocumentObject;
class xiiQtDocumentTreeModelAdapter;

/// This type is used to provide xiiDragDropHandler instances with all the important information for a drag & drop target
///
/// It is a reflected class such that one can derive and extend it, if necessary.
/// DragDrop handlers can then inspect whether it is a known extended type and cast to the type to get access to additional information.
class XII_EDITORFRAMEWORK_DLL xiiDragDropInfo : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDragDropInfo, xiiReflectedClass);

public:
  xiiDragDropInfo();

  const QMimeData* m_pMimeData;

  /// A string identifying into what context the object is dropped, e.g. "viewport" or "scenetree" etc.
  xiiString m_sTargetContext;

  /// The xiiDocument GUID
  xiiUuid m_TargetDocument;

  /// GUID of the xiiDocumentObject that is at the dropped position. May be invalid. Can be used to attach as a child, to modify the object itself or
  /// can be ignored.
  xiiUuid m_TargetObject;

  /// GUID of the xiiDocumentObject that may be used as the parent, if no other target is more important.
  xiiUuid m_ActiveParentObject;

  /// GUID of the xiiDocumentObject that is the more specific component (of m_TargetObject) that was dragged on. May be invalid.
  xiiUuid m_TargetComponent;

  /// World space position where the object is dropped. May be NaN.
  xiiVec3 m_vDropPosition;

  /// World space normal at the point where the object is dropped. May be NaN.
  xiiVec3 m_vDropNormal;

  /// Some kind of index / ID for the object that is at the drop location. For meshes this is the material index.
  xiiInt32 m_iTargetObjectSubID;

  /// If dropped on a scene tree, this may say as which child the object is supposed to be inserted. -1 if invalid (ie. append)
  xiiInt32 m_iTargetObjectInsertChildIndex;

  /// If dropped on a scene tree, this is the adapter for the target object.
  const xiiQtDocumentTreeModelAdapter* m_pAdapter = nullptr;

  bool m_bShiftKeyDown;
  bool m_bCtrlKeyDown;
};


/// After a xiiDragDropHandler has been chosen to handle an operation, it is queried once to fill out an instance of this type (or an extended
/// derived type) to enable configuring how xiiDragDropInfo is computed by the target.
class XII_EDITORFRAMEWORK_DLL xiiDragDropConfig : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDragDropConfig, xiiReflectedClass);

public:
  xiiDragDropConfig();

  /// Whether the currently selected objects (ie the dragged objects) should be considered for picking or not. Default is disabled.
  bool m_bPickSelectedObjects;
};
