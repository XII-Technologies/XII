/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineRestartSimulationMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineRestartSimulationMsg>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineLoopAnimationMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineLoopAnimationMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineSetMaterialsMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineSetMaterialsMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
