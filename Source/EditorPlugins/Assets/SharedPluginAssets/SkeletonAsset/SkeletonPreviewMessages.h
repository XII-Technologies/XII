/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Types/UniquePtr.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

/// Authoring metadata travels separately from the native runtime resource, including for background thumbnails.
class XII_SHAREDPLUGINASSETS_DLL xiiEditorEngineSkeletonPreviewMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineSkeletonPreviewMsg, xiiEditorEngineDocumentMsg);

public:
  xiiResult SetSkeleton(const xiiEditableSkeleton& skeleton);
  xiiResult GetSkeleton(xiiUniquePtr<xiiEditableSkeleton>& out_skeleton) const;

  xiiDynamicArray<xiiUInt8> m_Data;
};
