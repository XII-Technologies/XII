/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>

class xiiDragDropInfo;
class xiiDragDropConfig;

class XII_EDITORFRAMEWORK_DLL xiiDragDropHandler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDragDropHandler, xiiReflectedClass);

public:
  /// Returns whether the last call to BeginDragDropOperation() was successful and a handler is now in effect.
  static bool IsHandlerActive() { return s_pActiveDnD != nullptr; }

  /// Call this when a drag enter event occurs. Return value indicates whether a xiiDragDropHandler was found to handle the operation. If not,
  /// subsequent drag & drop updates are ignored.
  static bool BeginDragDropOperation(const xiiDragDropInfo* pInfo, xiiDragDropConfig* pConfigToFillOut = nullptr);

  /// Call this when a drag event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void UpdateDragDropOperation(const xiiDragDropInfo* pInfo);

  /// Call this when a drop event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void FinishDragDrop(const xiiDragDropInfo* pInfo);

  /// Call this when a drag leave event occurs. Ignored if BeginDragDropOperation() was not successful.
  static void CancelDragDrop();


  /// For targets that do not support full dragging, but only dropping on a single target, this allows to query whether there is a handler for
  /// the given target. See also DropOnly().
  static bool CanDropOnly(const xiiDragDropInfo* pInfo);

  /// Executes a complete drop action on a target that does not support continuous dragging. See also CanDropOnly().
  static bool DropOnly(const xiiDragDropInfo* pInfo);

public:
  xiiDragDropHandler();

protected:
  /// Used to ask a handler whether it knows how to handle a certain drag & drop situation.
  ///
  /// The return value is a priority. By default CanHandle should return 0 or 1. To override an existing handler, values larger than 1 may be returned
  /// to take precedence.
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const = 0;

  /// Potentially called by the drag drop target to request information about how to determine the xiiDragDropInfo data.
  virtual void RequestConfiguration(xiiDragDropConfig* pConfigToFillOut) {}

  /// Called shortly after CanHandle returned true to begin handling a drag operation.
  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) = 0;

  /// Called to update the drag operation with the latest state.
  virtual void OnDragUpdate(const xiiDragDropInfo* pInfo) = 0;

  /// Called when the drag operation leaves the designated area. The handler will be destroyed after this. It should clean up all temporary
  /// objects that it created before.
  virtual void OnDragCancel() = 0;

  /// Final call to finish the drag & drop operation. Handler is destroyed after this.
  virtual void OnDrop(const xiiDragDropInfo* pInfo) = 0;

private:
  static xiiDragDropHandler* FindDragDropHandler(const xiiDragDropInfo* pInfo);

  static xiiDragDropHandler* s_pActiveDnD;
};
