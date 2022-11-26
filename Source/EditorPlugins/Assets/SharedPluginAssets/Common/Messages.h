#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class XII_SHAREDPLUGINASSETS_DLL xiiEditorEngineRestartSimulationMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineRestartSimulationMsg, xiiEditorEngineDocumentMsg);

public:
};

class XII_SHAREDPLUGINASSETS_DLL xiiEditorEngineLoopAnimationMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineLoopAnimationMsg, xiiEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};

class XII_SHAREDPLUGINASSETS_DLL xiiEditorEngineSetMaterialsMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineSetMaterialsMsg, xiiEditorEngineDocumentMsg);

public:
  xiiHybridArray<xiiString, 16> m_Materials;
};
