/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct xiiManipulatorManagerEvent;
class xiiDocument;

class XII_EDITORFRAMEWORK_DLL xiiManipulatorAdapterRegistry
{
  XII_DECLARE_SINGLETON(xiiManipulatorAdapterRegistry);

public:
  xiiManipulatorAdapterRegistry();
  ~xiiManipulatorAdapterRegistry();

  xiiRttiMappedObjectFactory<xiiManipulatorAdapter> m_Factory;

  void QueryGridSettings(const xiiDocument* pDocument, xiiGridSettingsMsgToEngine& out_gridSettings);

private:
  void ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e);
  void ClearAdapters(const xiiDocument* pDocument);

  struct Data
  {
    xiiHybridArray<xiiManipulatorAdapter*, 8> m_Adapters;
  };

  xiiMap<const xiiDocument*, Data> m_DocumentAdapters;
};
