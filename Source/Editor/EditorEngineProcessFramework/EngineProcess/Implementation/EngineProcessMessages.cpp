/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Network/Reflection/NetworkAttributes.h>
#include <Foundation/Network/Reflection/NetworkTypeRegistry.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

namespace
{
  constexpr xiiUInt64 RenderConfigurationCodecId      = 0x5849494552434F4EULL;
  constexpr xiiUInt32 MaximumRenderConfigurationBytes = 1024 * 1024 + 4;

  xiiNetworkAttribute* RenderConfigurationField(xiiUInt64 uiField)
  {
    auto* attribute        = new xiiNetworkAttribute(uiField);
    attribute->m_uiCodecId = RenderConfigurationCodecId;
    return attribute;
  }

  class RenderConfigurationCodec final : public xiiNetworkCustomCodec
  {
  public:
    const xiiRTTI* GetType() const override { return xiiGetStaticRTTI<xiiRenderConfiguration>(); }
    xiiUInt64      GetWireId() const override { return RenderConfigurationCodecId; }
    xiiUInt32      GetVersion() const override { return xiiRenderConfiguration::s_uiSerializationVersion; }

    xiiNetworkDataError Encode(const xiiVariant& value, xiiNetworkBinaryWriter& writer) const override
    {
      xiiContiguousMemoryStreamStorage storage;
      xiiMemoryStreamWriter            stream(&storage);
      if (static_cast<const xiiRenderConfiguration*>(value.GetData())->Serialize(stream).Failed())
        return xiiNetworkDataError(xiiNetworkDataErrorCode::CustomCodecFailure);
      writer.Bytes(storage.GetContiguousMemoryRange(0));
      return writer.GetError();
    }

    xiiNetworkDataError Decode(xiiNetworkBinaryReader& reader, xiiNetworkDecodeBudget& budget, xiiVariant& out_value) const override
    {
      if (reader.GetRemaining() > MaximumRenderConfigurationBytes)
        return xiiNetworkDataError(xiiNetworkDataErrorCode::SizeLimit);
      const auto bytes    = reader.Bytes(reader.GetRemaining());
      const auto reserved = budget.Reserve(16ULL * bytes.GetCount() + sizeof(xiiRenderConfiguration));
      if (reserved.Failed())
        return reserved;
      xiiRawMemoryStreamReader stream(bytes.GetPtr(), bytes.GetCount());
      xiiRenderConfiguration*  config = GetType()->GetAllocator()->Allocate<xiiRenderConfiguration>();
      if (config->Deserialize(stream, GetVersion()).Failed() || stream.GetReadPosition() != bytes.GetCount())
      {
        GetType()->GetAllocator()->Deallocate(config);
        return xiiNetworkDataError(xiiNetworkDataErrorCode::CustomCodecFailure);
      }
      out_value.MoveTypedObject(config, GetType());
      return {};
    }
  };
} // namespace

xiiResult RegisterEditorProcessCodecs(xiiNetworkTypeRegistry& registry)
{
  xiiSharedPtr<xiiNetworkCustomCodec> codec = XII_DEFAULT_NEW(RenderConfigurationCodec);
  return registry.RegisterCodec(std::move(codec)).GetResult();
}

// clang-format off

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSyncWithProcessMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSyncWithProcessMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000001ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSyncWithProcessMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiSyncWithProcessMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000002ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// xiiEditorEngineMsg /////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineMsg, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUpdateReflectionTypeMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiUpdateReflectionTypeMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Descriptor", m_desc)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000003ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSetupProjectMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSetupProjectMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProjectDir", m_sProjectDir)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("PluginConfig", m_PluginConfig)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("Platform", m_sAssetProfile)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("DevicePixelRatio", m_fDevicePixelRatio)->AddAttributes(new xiiNetworkAttribute(6)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000004ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShutdownProcessMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiShutdownProcessMsgToEngine>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000005ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProjectReadyMsgToEditor, 3, xiiRTTIDefaultAllocator<xiiProjectReadyMsgToEditor> )
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Succeeded", m_bSucceeded)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Failure", m_sFailure)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("HardwareRayTracing", m_bHardwareRayTracing)->AddAttributes(new xiiNetworkAttribute(3)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000006ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleConfigMsgToEngine, 2, xiiRTTIDefaultAllocator<xiiSimpleConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Payload", m_sPayload)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("PayloadValue", m_fPayload)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000007ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSaveProfilingResponseToEditor, 2, xiiRTTIDefaultAllocator<xiiSaveProfilingResponseToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000008ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiReloadResourceMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiReloadResourceMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ID", m_sResourceID)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000009ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiResourceUpdateMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiResourceUpdateMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ID", m_sResourceID)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Data", m_Data)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000AULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRestoreResourceMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiRestoreResourceMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_sResourceType)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ID", m_sResourceID)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000BULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiChangeCVarMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiChangeCVarMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sCVarName)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Value", m_NewValue)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000CULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConsoleCmdMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiConsoleCmdMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Type", m_iType)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Cmd", m_sCommand)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000DULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConsoleCmdResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiConsoleCmdResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Result", m_sResult)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000EULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicStringEnumMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiDynamicStringEnumMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EnumName", m_sEnumName)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000000FULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpReplicationMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpReplicationMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("DocGuid", m_DocumentGuid)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Type", m_sReplicationType)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Data", m_ReplicationData)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000010ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpProgressMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpProgressMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Completion", m_fCompletion)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000011ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLongOpResultMsg, 1, xiiRTTIDefaultAllocator<xiiLongOpResultMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OpGuid", m_OperationGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Success", m_bSuccess)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Data", m_ResultData)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000012ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// xiiEditorEngineDocumentMsg /////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineDocumentMsg, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid)->AddAttributes(new xiiNetworkAttribute(1000)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentConfigMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiDocumentConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Int", m_iValue)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Float", m_fValue)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("String", m_sValue)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000013ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineViewMsg, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID)->AddAttributes(new xiiNetworkAttribute(1001)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentOpenMsgToEngine, 2, xiiRTTIDefaultAllocator<xiiDocumentOpenMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("DocumentType", m_sDocumentType)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000014ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentClearMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiDocumentClearMsgToEngine>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000015ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentOpenResponseMsgToEditor, 3, xiiRTTIDefaultAllocator<xiiDocumentOpenResponseMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Succeeded", m_bSucceeded)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Failure", m_sFailure)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000016ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewDestroyedMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewDestroyedMsgToEngine>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000017ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewDestroyedResponseMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewDestroyedResponseMsgToEditor>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000018ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewFrameMsgToEditor, 3, xiiRTTIDefaultAllocator<xiiViewFrameMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FrameID", m_uiFrameID)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("TextureID", m_uiTextureID)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("SlotIndex", m_uiSlotIndex)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("RenderWidth", m_uiRenderWidth)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("RenderHeight", m_uiRenderHeight)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("EffectiveRenderConfiguration", m_EffectiveRenderConfiguration)->AddAttributes(RenderConfigurationField(6)),
    XII_MEMBER_PROPERTY("RenderConfigurationError", m_sRenderConfigurationError)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiNetworkAttribute(8)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiNetworkAttribute(9)),
    XII_MEMBER_PROPERTY("Memory", m_Memory)->AddAttributes(new xiiNetworkAttribute(10)),
    XII_MEMBER_PROPERTY("Pixels", m_Pixels)->AddAttributes(new xiiNetworkAttribute(11)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000019ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewFrameConsumedMsgToEngine, 2, xiiRTTIDefaultAllocator<xiiViewFrameConsumedMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FrameID", m_uiFrameID)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("SlotIndex", m_uiSlotIndex)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("SharedTextureAccepted", m_bSharedTextureAccepted)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001AULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewRedrawMsgToEngine, 4, xiiRTTIDefaultAllocator<xiiViewRedrawMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UseSharedTexture", m_bUseSharedTexture)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("DevicePixelRatio", m_fDevicePixelRatio)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice)->AddAttributes(new xiiNetworkAttribute(8)),
    XII_MEMBER_PROPERTY("ShowFrameStats", m_bShowFrameStats)->AddAttributes(new xiiNetworkAttribute(9)),
    XII_MEMBER_PROPERTY("RenderConfiguration", m_RenderConfiguration)->AddAttributes(RenderConfigurationField(10)),
    XII_MEMBER_PROPERTY("ShowNavigationGizmo", m_bShowNavigationGizmo)->AddAttributes(new xiiNetworkAttribute(11)),
    XII_MEMBER_PROPERTY("NavigationHovered", m_bNavigationHovered)->AddAttributes(new xiiNetworkAttribute(12)),
    XII_MEMBER_PROPERTY("NavigationHoveredAxis", m_iNavigationHoveredAxis)->AddAttributes(new xiiNetworkAttribute(13)),
    XII_MEMBER_PROPERTY("NearPlane", m_fNearPlane)->AddAttributes(new xiiNetworkAttribute(14)),
    XII_MEMBER_PROPERTY("FarPlane", m_fFarPlane)->AddAttributes(new xiiNetworkAttribute(15)),
    XII_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim)->AddAttributes(new xiiNetworkAttribute(16)),
    XII_MEMBER_PROPERTY("Position", m_vPosition)->AddAttributes(new xiiNetworkAttribute(17)),
    XII_MEMBER_PROPERTY("Forwards", m_vDirForwards)->AddAttributes(new xiiNetworkAttribute(18)),
    XII_MEMBER_PROPERTY("Up", m_vDirUp)->AddAttributes(new xiiNetworkAttribute(19)),
    XII_MEMBER_PROPERTY("Right", m_vDirRight)->AddAttributes(new xiiNetworkAttribute(20)),
    XII_MEMBER_PROPERTY("ViewMat", m_ViewMatrix)->AddAttributes(new xiiNetworkAttribute(21)),
    XII_MEMBER_PROPERTY("ProjMat", m_ProjMatrix)->AddAttributes(new xiiNetworkAttribute(22)),
    XII_ENUM_MEMBER_PROPERTY("CameraMode", xiiCameraMode, m_CameraMode)->AddAttributes(new xiiNetworkAttribute(23)),
    XII_ENUM_MEMBER_PROPERTY("RenderMode", xiiViewRenderMode, m_RenderMode)->AddAttributes(new xiiNetworkAttribute(24)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001BULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewScreenshotMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewScreenshotMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("File", m_sOutputFile)->AddAttributes(new xiiNetworkAttribute(1))
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001CULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActivateRemoteViewMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiActivateRemoteViewMsgToEngine>)
{
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001DULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEntityMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiEntityMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Change", m_Change)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001EULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleDocumentConfigMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSimpleDocumentConfigMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Payload1", m_sPayload)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Payload2", m_PayloadValue)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000001FULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimpleDocumentConfigMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiSimpleDocumentConfigMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Payload1", m_sPayload)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Payload2", m_PayloadValue)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000020ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExportDocumentMsgToEngine, 2, xiiRTTIDefaultAllocator<xiiExportDocumentMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("OutputFile", m_sOutputFile)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("AssetHash", m_uiAssetHash)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("AssetVersion", m_uiVersion)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000021ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExportDocumentMsgToEditor, 2, xiiRTTIDefaultAllocator<xiiExportDocumentMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000022ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateThumbnailMsgToEngine, 2, xiiRTTIDefaultAllocator<xiiCreateThumbnailMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000023ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateThumbnailMsgToEditor, 2, xiiRTTIDefaultAllocator<xiiCreateThumbnailMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestId", m_RequestId)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000024ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewPickingMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewPickingMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestID", m_uiRequestID)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("PickPosX", m_uiPickPosX)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("PickPosY", m_uiPickPosY)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("PickGizmos", m_bPickGizmos)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000025ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewPickingResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewPickingResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestID", m_uiRequestID)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("OtherGuid", m_OtherGuid)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("PartIndex", m_uiPartIndex)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("PickedPos", m_vPickedPosition)->AddAttributes(new xiiNetworkAttribute(6)),
    XII_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal)->AddAttributes(new xiiNetworkAttribute(7)),
    XII_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition)->AddAttributes(new xiiNetworkAttribute(8)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000026ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewMarqueePickingMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewMarqueePickingMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1)->AddAttributes(new xiiNetworkAttribute(4)),
    XII_MEMBER_PROPERTY("what", m_uiWhatToDo)->AddAttributes(new xiiNetworkAttribute(5)),
    XII_MEMBER_PROPERTY("aid", m_uiActionIdentifier)->AddAttributes(new xiiNetworkAttribute(6)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000027ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewMarqueePickingResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiViewMarqueePickingResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("what", m_uiWhatToDo)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("aid", m_uiActionIdentifier)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000028ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewHighlightMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiViewHighlightMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HighlightObject", m_HighlightObject)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000029ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLogMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiLogMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Entry", m_Entry)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002AULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCVarMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiCVarMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Plugin", m_sPlugin)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Desc", m_sDescription)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002BULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorEngineSyncObjectMsg, 1, xiiRTTIDefaultAllocator<xiiEditorEngineSyncObjectMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ObjectType", m_sObjectType)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData)->AddAttributes(new xiiNetworkAttribute(3)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002CULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectTagMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiObjectTagMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Tag", m_sTag)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Set", m_bSetTag)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002DULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectSelectionMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiObjectSelectionMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Selection", m_sSelection)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002EULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimulationSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiSimulationSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x584949450000002FULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGridSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GridDensity", m_fGridDensity)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("GridCenter", m_vGridCenter)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000030ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGlobalSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGlobalSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000031ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldSettingsMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiWorldSettingsMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000032ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameModeMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiGameModeMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Run", m_bEnablePTG)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("UsePos", m_bUseStartPosition)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("Pos", m_vStartPosition)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Dir", m_vStartDirection)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000033ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameModeMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiGameModeMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Run", m_bRunningPTG)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000034ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuerySelectionBBoxMsgToEngine, 1, xiiRTTIDefaultAllocator<xiiQuerySelectionBBoxMsgToEngine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Purpose", m_iPurpose)->AddAttributes(new xiiNetworkAttribute(2)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000035ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiQuerySelectionBBoxResultMsgToEditor, 1, xiiRTTIDefaultAllocator<xiiQuerySelectionBBoxResultMsgToEditor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Center", m_vCenter)->AddAttributes(new xiiNetworkAttribute(1)),
    XII_MEMBER_PROPERTY("Extents", m_vHalfExtents)->AddAttributes(new xiiNetworkAttribute(2)),
    XII_MEMBER_PROPERTY("ViewID", m_uiViewID)->AddAttributes(new xiiNetworkAttribute(3)),
    XII_MEMBER_PROPERTY("Purpose", m_iPurpose)->AddAttributes(new xiiNetworkAttribute(4)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000036ULL) } XII_END_ATTRIBUTES;
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
    XII_MEMBER_PROPERTY("Objects", m_Objects)->AddAttributes(new xiiNetworkAttribute(1)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiNetworkIdentityAttribute(0x5849494500000037ULL) } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
