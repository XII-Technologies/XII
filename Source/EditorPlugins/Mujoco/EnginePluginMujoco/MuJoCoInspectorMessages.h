/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EnginePluginMujoco/EnginePluginMujocoDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Types/Variant.h>

enum class xiiMuJoCoInspectorCommand : xiiUInt8
{
  Inspect,
  Metadata,
  SetJointPosition,
  SetJointVelocity,
  ResetJoint,
  SetActuatorControls,
  ResetSimulation,
  SingleStep
};

/// Requests use the document's existing synchronized-object channel. Only transient XII handles,
/// generation-checked native indices, and copied values cross processes; no native pointers.
class XII_ENGINEPLUGINMUJOCO_DLL xiiMuJoCoInspectorSyncObject final : public xiiEditorEngineSyncObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoInspectorSyncObject, xiiEditorEngineSyncObject);

public:
  xiiMuJoCoInspectorSyncObject();
  ~xiiMuJoCoInspectorSyncObject();
  void UpdateForEngine(xiiWorld* pWorld) override;

  xiiUuid                 m_EngineEpoch; ///< Mutations must acknowledge the engine-created epoch, preventing replay after restart/reset.
  xiiUInt64               m_uiRequest     = 0;
  xiiUInt64               m_uiModelToken  = 0; ///< Transient XII component handle, never an authored reference.
  xiiUInt64               m_uiGeneration  = 0;
  xiiUInt64               m_uiOffset      = 0;
  xiiUInt32               m_uiModes       = 0;
  xiiInt32                m_iElementType  = -1;
  xiiInt32                m_iElementIndex = -1;
  xiiUInt8                m_uiCommand     = 0;
  xiiDynamicArray<double> m_Values;
  xiiUInt64               m_uiCommandModel    = 0;
  xiiInt32                m_iCommandType      = -1;
  xiiInt32                m_iCommandIndex     = -1;
  bool                    m_bPreview          = false;
  bool                    m_bAuthoring        = true;
  bool                    m_bDeferComposition = false;
  bool                    m_bContacts         = false;

private:
  xiiUuid   m_BackendEpoch;
  xiiUInt64 m_uiProcessedRequest = 0;
  xiiWorld* m_pWorld             = nullptr;
};

/// Bounded metadata pages and one selected element's live values. A reply is consumed only by its
/// session and request serial. Model generation and backend epoch invalidate all cached selections.
class XII_ENGINEPLUGINMUJOCO_DLL xiiMuJoCoInspectorResponse final : public xiiEditorEngineDocumentMsg
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMuJoCoInspectorResponse, xiiEditorEngineDocumentMsg);

public:
  xiiUuid              m_Session;
  xiiUuid              m_EngineEpoch;
  xiiUInt64            m_uiRequest    = 0;
  xiiUInt64            m_uiGeneration = 0;
  xiiVariantDictionary m_Data;
};
