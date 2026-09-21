/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

///////////////////////////////////// xiiProcessMessages /////////////////////////////////////



///////////////////////////////////// xiiEditorEngineMsg /////////////////////////////////////

/// Base class for all messages between editor and engine that are not bound to any document
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineMsg : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineMsg, xiiProcessMessage);

public:
  xiiEditorEngineMsg() = default;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiUpdateReflectionTypeMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUpdateReflectionTypeMsgToEditor, xiiEditorEngineMsg);

public:
  // Mutable because it is eaten up by xiiPhantomRttiManager.
  mutable xiiReflectedTypeDescriptor m_desc;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSetupProjectMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSetupProjectMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString                      m_sProjectDir;
  xiiApplicationFileSystemConfig m_FileSystemConfig;
  xiiApplicationPluginConfig     m_PluginConfig;
  xiiString                      m_sFileserveAddress; ///< Optionally used for remote processes to tell them with which IP address to connect to the host
  xiiString                      m_sAssetProfile;
  float                          m_fDevicePixelRatio = 1.0f;
};

/// Sent to remote processes to shut them down.
/// Local processes are simply killed through QProcess::close, but remote processes have to close themselves.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiShutdownProcessMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShutdownProcessMsgToEngine, xiiEditorEngineMsg);

public:
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiProjectReadyMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProjectReadyMsgToEditor, xiiEditorEngineMsg);

public:
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSimpleConfigMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleConfigMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString m_sWhatToDo;
  xiiString m_sPayload;
  double    m_fPayload;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSaveProfilingResponseToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSaveProfilingResponseToEditor, xiiEditorEngineMsg);

public:
  xiiString m_sProfilingFile;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiReloadResourceMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiReloadResourceMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString m_sResourceType;
  xiiString m_sResourceID;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiResourceUpdateMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiResourceUpdateMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString     m_sResourceType;
  xiiString     m_sResourceID;
  xiiDataBuffer m_Data;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiRestoreResourceMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRestoreResourceMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString m_sResourceType;
  xiiString m_sResourceID;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiChangeCVarMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiChangeCVarMsgToEngine, xiiEditorEngineMsg);

public:
  xiiString  m_sCVarName;
  xiiVariant m_NewValue;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiConsoleCmdMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConsoleCmdMsgToEngine, xiiEditorEngineMsg);

public:
  xiiInt8   m_iType; // 0 = execute, 1 = auto complete
  xiiString m_sCommand;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiConsoleCmdResultMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConsoleCmdResultMsgToEditor, xiiEditorEngineMsg);

public:
  xiiString m_sResult;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiDynamicStringEnumMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDynamicStringEnumMsgToEditor, xiiEditorEngineMsg);

public:
  xiiString                    m_sEnumName;
  xiiHybridArray<xiiString, 8> m_EnumValues;
};

///////////////////////////////////// xiiEditorEngineDocumentMsg /////////////////////////////////////

/// Base class for all messages that are tied to some document.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineDocumentMsg : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineDocumentMsg, xiiProcessMessage);

public:
  xiiUuid m_DocumentGuid;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSimpleDocumentConfigMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleDocumentConfigMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiString  m_sWhatToDo;
  xiiString  m_sPayload;
  xiiVariant m_PayloadValue;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSimpleDocumentConfigMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimpleDocumentConfigMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  xiiString  m_sWhatToDo;
  xiiString  m_sPayload;
  xiiVariant m_PayloadValue;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSyncWithProcessMsgToEngine : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSyncWithProcessMsgToEngine, xiiProcessMessage);

public:
  xiiUInt32 m_uiRedrawCount;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSyncWithProcessMsgToEditor : public xiiProcessMessage
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSyncWithProcessMsgToEditor, xiiProcessMessage);

public:
  xiiUInt32 m_uiRedrawCount;
};


class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineViewMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineViewMsg, xiiEditorEngineDocumentMsg);

public:
  xiiEditorEngineViewMsg() { m_uiViewID = 0xFFFFFFFF; }

  xiiUInt32 m_uiViewID;
};

/// For very simple uses cases where a custom message would be too much
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiDocumentConfigMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentConfigMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiString m_sWhatToDo;
  int       m_iValue;
  float     m_fValue;
  xiiString m_sValue;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiDocumentOpenMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentOpenMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiDocumentOpenMsgToEngine() { m_bDocumentOpen = false; }

  bool       m_bDocumentOpen;
  xiiString  m_sDocumentType;
  xiiVariant m_DocumentMetaData;
};

/// Used to reset the engine side to an empty document before sending the full document state over
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiDocumentClearMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentClearMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiDocumentClearMsgToEngine() = default;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiDocumentOpenResponseMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentOpenResponseMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  xiiDocumentOpenResponseMsgToEditor() = default;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewDestroyedMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewDestroyedMsgToEngine, xiiEditorEngineViewMsg);
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewDestroyedResponseMsgToEditor : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewDestroyedResponseMsgToEditor, xiiEditorEngineViewMsg);
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewRedrawMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewRedrawMsgToEngine, xiiEditorEngineViewMsg);

public:
  xiiUInt64 m_uiHWND;
  xiiUInt16 m_uiWindowWidth;
  xiiUInt16 m_uiWindowHeight;
  bool      m_bUpdatePickingData;
  bool      m_bEnablePickingSelected;
  bool      m_bEnablePickTransparent;
  bool      m_bUseCameraTransformOnDevice = true;

  float                      m_fNearPlane;
  float                      m_fFarPlane;
  float                      m_fFovOrDim;
  xiiEnum<xiiCameraMode>     m_CameraMode;
  xiiEnum<xiiViewRenderMode> m_RenderMode;

  xiiVec3 m_vPosition;
  xiiVec3 m_vDirForwards;
  xiiVec3 m_vDirUp;
  xiiVec3 m_vDirRight;
  xiiMat4 m_ViewMatrix;
  xiiMat4 m_ProjMatrix;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewScreenshotMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewScreenshotMsgToEngine, xiiEditorEngineViewMsg);

public:
  xiiString m_sOutputFile;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiActivateRemoteViewMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiActivateRemoteViewMsgToEngine, xiiEditorEngineViewMsg);

public:
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEntityMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEntityMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiObjectChange m_Change;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiExportDocumentMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExportDocumentMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiExportDocumentMsgToEngine() = default;

  xiiString m_sOutputFile;
  xiiUInt64 m_uiAssetHash = 0;
  xiiUInt16 m_uiVersion   = 0;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiExportDocumentMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExportDocumentMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  bool      m_bOutputSuccess = false;
  xiiString m_sFailureMsg;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiCreateThumbnailMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateThumbnailMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiUInt16                    m_uiWidth  = 0;
  xiiUInt16                    m_uiHeight = 0;
  xiiHybridArray<xiiString, 1> m_ViewExcludeTags;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiCreateThumbnailMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCreateThumbnailMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  xiiCreateThumbnailMsgToEditor() = default;
  xiiDataBuffer m_ThumbnailData; ///< Raw 8-bit RGBA data (256x256x4 bytes)
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewPickingMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewPickingMsgToEngine, xiiEditorEngineViewMsg);

public:
  xiiUInt16 m_uiPickPosX;
  xiiUInt16 m_uiPickPosY;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewPickingResultMsgToEditor : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewPickingResultMsgToEditor, xiiEditorEngineViewMsg);

public:
  xiiUuid   m_ObjectGuid;
  xiiUuid   m_ComponentGuid;
  xiiUuid   m_OtherGuid;
  xiiUInt32 m_uiPartIndex;

  xiiVec3 m_vPickedPosition;
  xiiVec3 m_vPickedNormal;
  xiiVec3 m_vPickingRayStartPosition;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewMarqueePickingMsgToEngine : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewMarqueePickingMsgToEngine, xiiEditorEngineViewMsg);

public:
  xiiUInt16 m_uiPickPosX0;
  xiiUInt16 m_uiPickPosY0;

  xiiUInt16 m_uiPickPosX1;
  xiiUInt16 m_uiPickPosY1;

  xiiUInt8  m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  xiiUInt32 m_uiActionIdentifier;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewMarqueePickingResultMsgToEditor : public xiiEditorEngineViewMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewMarqueePickingResultMsgToEditor, xiiEditorEngineViewMsg);

public:
  xiiDynamicArray<xiiUuid> m_ObjectGuids;
  xiiUInt8                 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  xiiUInt32                m_uiActionIdentifier;
};


class xiiEditorEngineConnection;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiViewHighlightMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewHighlightMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLogMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogMsgToEditor, xiiEditorEngineMsg);

public:
  xiiLogEntry m_Entry;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiCVarMsgToEditor : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCVarMsgToEditor, xiiEditorEngineMsg);

public:
  xiiString  m_sName;
  xiiString  m_sPlugin;
  xiiString  m_sDescription;
  xiiVariant m_Value;
};


class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpReplicationMsg : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpReplicationMsg, xiiEditorEngineMsg);

public:
  xiiUuid       m_OperationGuid;
  xiiUuid       m_DocumentGuid;
  xiiString     m_sReplicationType;
  xiiDataBuffer m_ReplicationData;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpProgressMsg : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpProgressMsg, xiiEditorEngineMsg);

public:
  xiiUuid m_OperationGuid;
  float   m_fCompletion;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpResultMsg : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpResultMsg, xiiEditorEngineMsg);

public:
  xiiUuid       m_OperationGuid;
  bool          m_bSuccess;
  xiiDataBuffer m_ResultData;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEditorEngineSyncObjectMsg : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorEngineSyncObjectMsg, xiiEditorEngineDocumentMsg);

public:
  xiiUuid       m_ObjectGuid;
  xiiString     m_sObjectType;
  xiiDataBuffer m_ObjectData;

  const xiiDataBuffer& GetObjectData() const { return m_ObjectData; }
  void                 SetObjectData(const xiiDataBuffer& s) { m_ObjectData = s; }
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiObjectTagMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectTagMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiObjectTagMsgToEngine()
  {
    m_bSetTag             = false;
    m_bApplyOnAllChildren = false;
  }

  xiiUuid   m_ObjectGuid;
  xiiString m_sTag;
  bool      m_bSetTag;
  bool      m_bApplyOnAllChildren;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiObjectSelectionMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectSelectionMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiString m_sSelection;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiSimulationSettingsMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimulationSettingsMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  bool  m_bSimulateWorld   = false;
  float m_fSimulationSpeed = 1.0f;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGridSettingsMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridSettingsMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  float   m_fGridDensity = 0.0f;
  xiiVec3 m_vGridCenter;
  xiiVec3 m_vGridTangent1;
  xiiVec3 m_vGridTangent2;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGlobalSettingsMsgToEngine : public xiiEditorEngineMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGlobalSettingsMsgToEngine, xiiEditorEngineMsg);

public:
  float m_fGizmoScale = 0.0f;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiWorldSettingsMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiWorldSettingsMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  bool m_bRenderOverlay        = false;
  bool m_bRenderShapeIcons     = false;
  bool m_bRenderSelectionBoxes = false;
  bool m_bAddAmbientLight      = false;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGameModeMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameModeMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  bool    m_bEnablePTG        = false;
  bool    m_bUseStartPosition = false;
  xiiVec3 m_vStartPosition;
  xiiVec3 m_vStartDirection;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGameModeMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameModeMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiQuerySelectionBBoxMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiQuerySelectionBBoxMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiUInt32 m_uiViewID; /// passed through to xiiQuerySelectionBBoxResultMsgToEditor
  xiiInt32  m_iPurpose; /// passed through to xiiQuerySelectionBBoxResultMsgToEditor
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiQuerySelectionBBoxResultMsgToEditor : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiQuerySelectionBBoxResultMsgToEditor, xiiEditorEngineDocumentMsg);

public:
  xiiVec3 m_vCenter;
  xiiVec3 m_vHalfExtents;

  xiiUInt32 m_uiViewID; /// passed through from xiiQuerySelectionBBoxMsgToEngine
  xiiInt32  m_iPurpose; /// passed through from xiiQuerySelectionBBoxMsgToEngine
};

/// Send between editor documents, such that one document can know about objects in another document.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGatherObjectsOfTypeMsgInterDoc : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGatherObjectsOfTypeMsgInterDoc, xiiReflectedClass);

public:
  const xiiRTTI* m_pType;

  struct Result
  {
    const xiiDocument* m_pDocument;
    xiiUuid            m_ObjectGuid;
    xiiString          m_sDisplayName;
  };

  xiiDynamicArray<Result> m_Results;
};

/// Send by the editor scene document to all other editor documents, to gather on which objects debug visualization should be enabled during
/// play-the-game.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGatherObjectsForDebugVisMsgInterDoc : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGatherObjectsForDebugVisMsgInterDoc, xiiReflectedClass);

public:
  xiiDynamicArray<xiiUuid> m_Objects;
};

/// Send by the editor scene document to the runtime scene document, to tell it about the poll results (see xiiGatherObjectsForDebugVisMsgInterDoc).
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiObjectsForDebugVisMsgToEngine : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectsForDebugVisMsgToEngine, xiiEditorEngineDocumentMsg);

public:
  xiiDataBuffer m_Objects;
};
