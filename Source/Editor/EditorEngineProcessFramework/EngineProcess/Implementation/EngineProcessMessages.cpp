/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

// clang-format off

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSyncWithProcessMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSyncWithProcessMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSyncWithProcessMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiSyncWithProcessMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// xiiEditorEngineMsg /////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineMsg, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUpdateReflectionTypeMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiUpdateReflectionTypeMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Descriptor", m_desc),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetupProjectMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSetupProjectMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    XII_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig),
    XII_MEMBER_PROPERTY("PluginConfig", m_PluginConfig),
    XII_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress),
    XII_MEMBER_PROPERTY("Platform", m_sAssetProfile),
    XII_MEMBER_PROPERTY("DevicePixelRatio", m_fDevicePixelRatio),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShutdownProcessMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiShutdownProcessMsgToEngine>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProjectReadyMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiProjectReadyMsgToEditor> )
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleConfigMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSimpleConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    XII_MEMBER_PROPERTY("Payload", m_sPayload),
    XII_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSaveProfilingResponseToEditor, 1, xiiRTTIDefaultAllocator<xiiSaveProfilingResponseToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReloadResourceMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiReloadResourceMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType),
    XII_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiResourceUpdateMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiResourceUpdateMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType),
    XII_MEMBER_PROPERTY("ID", m_sResourceID),
    XII_MEMBER_PROPERTY("Data", m_Data),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRestoreResourceMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiRestoreResourceMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType),
    XII_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiChangeCVarMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiChangeCVarMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sCVarName),
    XII_MEMBER_PROPERTY("Value", m_NewValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConsoleCmdMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiConsoleCmdMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_iType),
    XII_MEMBER_PROPERTY("Cmd", m_sCommand),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConsoleCmdResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiConsoleCmdResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Result", m_sResult),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicStringEnumMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiDynamicStringEnumMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EnumName", m_sEnumName),
    XII_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpReplicationMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpReplicationMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    XII_MEMBER_PROPERTY("DocGuid", m_DocumentGuid),
    XII_MEMBER_PROPERTY("Type", m_sReplicationType),
    XII_MEMBER_PROPERTY("Data", m_ReplicationData),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpProgressMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpProgressMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    XII_MEMBER_PROPERTY("Completion", m_fCompletion),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpResultMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpResultMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    XII_MEMBER_PROPERTY("Success", m_bSuccess),
    XII_MEMBER_PROPERTY("Data", m_ResultData),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// xiiEditorEngineDocumentMsg /////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineDocumentMsg, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentConfigMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiDocumentConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    XII_MEMBER_PROPERTY("Int", m_iValue),
    XII_MEMBER_PROPERTY("Float", m_fValue),
    XII_MEMBER_PROPERTY("String", m_sValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineViewMsg, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentOpenMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiDocumentOpenMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
    XII_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    XII_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentClearMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiDocumentClearMsgToEngine>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentOpenResponseMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiDocumentOpenResponseMsgToEditor> )
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewDestroyedMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewDestroyedMsgToEngine>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewDestroyedResponseMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewDestroyedResponseMsgToEditor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewRedrawMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewRedrawMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HWND", m_uiHWND),
    XII_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    XII_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
    XII_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData),
    XII_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected),
    XII_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent),
    XII_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice),
    XII_MEMBER_PROPERTY("CameraMode", m_iCameraMode),
    XII_MEMBER_PROPERTY("NearPlane", m_fNearPlane),
    XII_MEMBER_PROPERTY("FarPlane", m_fFarPlane),
    XII_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim),
    XII_MEMBER_PROPERTY("Position", m_vPosition),
    XII_MEMBER_PROPERTY("Forwards", m_vDirForwards),
    XII_MEMBER_PROPERTY("Up", m_vDirUp),
    XII_MEMBER_PROPERTY("Right", m_vDirRight),
    XII_MEMBER_PROPERTY("ViewMat", m_ViewMatrix),
    XII_MEMBER_PROPERTY("ProjMat", m_ProjMatrix),
    XII_MEMBER_PROPERTY("RenderMode", m_uiRenderMode),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewScreenshotMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewScreenshotMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sOutputFile)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActivateRemoteViewMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiActivateRemoteViewMsgToEngine>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEntityMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiEntityMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Change", m_change),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleDocumentConfigMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSimpleDocumentConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    XII_MEMBER_PROPERTY("Payload1", m_sPayload),
    XII_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleDocumentConfigMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiSimpleDocumentConfigMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    XII_MEMBER_PROPERTY("Payload1", m_sPayload),
    XII_MEMBER_PROPERTY("Payload2", m_PayloadValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExportDocumentMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiExportDocumentMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OutputFile", m_sOutputFile),
    XII_MEMBER_PROPERTY("AssetHash", m_uiAssetHash),
    XII_MEMBER_PROPERTY("AssetVersion", m_uiVersion),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExportDocumentMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiExportDocumentMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess),
    XII_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateThumbnailMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiCreateThumbnailMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Width", m_uiWidth),
    XII_MEMBER_PROPERTY("Height", m_uiHeight),
    XII_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateThumbnailMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiCreateThumbnailMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewPickingMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewPickingMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    XII_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewPickingResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewPickingResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    XII_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    XII_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    XII_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    XII_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    XII_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    XII_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewMarqueePickingMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewMarqueePickingMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0),
    XII_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0),
    XII_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1),
    XII_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1),
    XII_MEMBER_PROPERTY("what", m_uiWhatToDo),
    XII_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewMarqueePickingResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewMarqueePickingResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids),
    XII_MEMBER_PROPERTY("what", m_uiWhatToDo),
    XII_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewHighlightMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewHighlightMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiLogMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Entry", m_Entry),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCVarMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiCVarMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Plugin", m_sPlugin),
    XII_MEMBER_PROPERTY("Desc", m_sDescription),
    XII_MEMBER_PROPERTY("Value", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineSyncObjectMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineSyncObjectMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    XII_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    XII_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectTagMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiObjectTagMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    XII_MEMBER_PROPERTY("Tag", m_sTag),
    XII_MEMBER_PROPERTY("Set", m_bSetTag),
    XII_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectSelectionMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiObjectSelectionMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Selection", m_sSelection),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimulationSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSimulationSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld),
    XII_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGridSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GridDensity", m_fGridDensity),
    XII_MEMBER_PROPERTY("GridCenter", m_vGridCenter),
    XII_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1),
    XII_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGlobalSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGlobalSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiWorldSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay),
    XII_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons),
    XII_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes),
    XII_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameModeMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGameModeMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Run", m_bEnablePTG),
    XII_MEMBER_PROPERTY("UsePos", m_bUseStartPosition),
    XII_MEMBER_PROPERTY("Pos", m_vStartPosition),
    XII_MEMBER_PROPERTY("Dir", m_vStartDirection),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameModeMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiGameModeMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Run", m_bRunningPTG),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuerySelectionBBoxMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiQuerySelectionBBoxMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID),
    XII_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuerySelectionBBoxResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiQuerySelectionBBoxResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Center", m_vCenter),
    XII_MEMBER_PROPERTY("Extents", m_vHalfExtents),
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID),
    XII_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGatherObjectsOfTypeMsgInterDoc, 1, xiiRTTIDefaultAllocator<xiiGatherObjectsOfTypeMsgInterDoc>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGatherObjectsForDebugVisMsgInterDoc, 1, xiiRTTIDefaultAllocator<xiiGatherObjectsForDebugVisMsgInterDoc>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectsForDebugVisMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiObjectsForDebugVisMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Objects", m_Objects),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
