#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct xiiVisualizerManagerEvent;
class xiiDocument;

class XII_EDITORFRAMEWORK_DLL xiiVisualizerAdapterRegistry
{
  XII_DECLARE_SINGLETON(xiiVisualizerAdapterRegistry);

public:
  xiiVisualizerAdapterRegistry();
  ~xiiVisualizerAdapterRegistry();

  xiiRttiMappedObjectFactory<xiiVisualizerAdapter> m_Factory;

private:
  void VisualizerManagerEventHandler(const xiiVisualizerManagerEvent& e);
  void ClearAdapters(const xiiDocument* pDocument);
  void CreateAdapters(const xiiDocument* pDocument, const xiiDocumentObject* pObject);

  struct Data
  {
    xiiHybridArray<xiiVisualizerAdapter*, 8> m_Adapters;
  };

  xiiMap<const xiiDocument*, Data> m_DocumentAdapters;
};
