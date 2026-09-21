/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class xiiVisualizerAttribute;
class xiiDocumentObject;
struct xiiDocumentObjectPropertyEvent;
struct xiiQtDocumentWindowEvent;
class xiiObjectAccessorBase;

/// Base class for the editor side code that sets up a 'visualizer' for object properties.
///
/// Typically visualizers are configured with xiiVisualizerAttribute's on component types.
/// The adapter reads the attribute values and sets up the necessary code to render them in the engine.
/// This is usually achieved by creating xiiEngineGizmoHandle objects (which get automatically synchronized
/// with the engine process).
/// The adapter then reacts to editor side object changes and adjusts the engine side representation
/// as needed.
class XII_EDITORFRAMEWORK_DLL xiiVisualizerAdapter
{
public:
  xiiVisualizerAdapter();
  virtual ~xiiVisualizerAdapter();

  void SetVisualizer(const xiiVisualizerAttribute* pAttribute, const xiiDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e);

protected:
  virtual xiiTransform       GetObjectTransform() const;
  xiiObjectAccessorBase*     GetObjectAccessor() const;
  const xiiAbstractProperty* GetProperty(const char* szProperty) const;

  /// Called to actually properly set up the adapter. All setup code is implemented here.
  virtual void Finalize() = 0;
  /// Called when object properties have changed and the visualizer may need to react.
  virtual void Update() = 0;
  /// Called when the object has been moved somehow. More light weight than a full update.
  virtual void UpdateGizmoTransform() = 0;

  bool                          m_bVisualizerIsVisible;
  const xiiVisualizerAttribute* m_pVisualizerAttr;
  const xiiDocumentObject*      m_pObject;
};
