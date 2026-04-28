/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/StateMachineAsset/StateMachineContext.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachineBuiltins.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineContext, 1, xiiRTTIDefaultAllocator<xiiStateMachineContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "StateMachine"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineContext::xiiStateMachineContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

xiiEngineProcessViewContext* xiiStateMachineContext::CreateViewContext()
{
  XII_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void xiiStateMachineContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_ASSERT_DEV(false, "Should not be called");
}

xiiStatus xiiStateMachineContext::ExportDocument(const xiiExportDocumentMsgToEngine* pMsg)
{
  xiiDynamicArray<xiiUuid>                    nodeUuids;
  xiiDynamicArray<xiiStateMachineNodeBase*>   nodes;
  xiiDynamicArray<xiiStateMachineConnection*> connections;

  m_Context.GetObjectsByType(nodes, &nodeUuids);
  m_Context.GetObjectsByType(connections);

  xiiStateMachineDescription       desc;
  xiiHashTable<xiiUuid, xiiUInt32> nodeUuidToStateIndex;
  xiiSet<xiiString>                stateNames;

  auto AddState = [&](const xiiStateMachineNode* pNode, const xiiUuid& uuid) -> xiiStatus {
    const xiiString& name = pNode->m_sName;
    if (stateNames.Contains(name))
    {
      return xiiStatus(xiiFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    xiiUniquePtr<xiiStateMachineState> pState = xiiUniquePtr<xiiStateMachineState>(pNode->m_pType, nullptr);
    if (pState == nullptr)
    {
      pState = XII_DEFAULT_NEW(xiiStateMachineState_Empty);
    }

    if (pState->GetName().IsEmpty())
    {
      pState->SetName(name);
    }

    const xiiUInt32 uiStateIndex = desc.AddState(std::move(pState));
    nodeUuidToStateIndex.Insert(uuid, uiStateIndex);

    return XII_SUCCESS;
  };

  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = xiiDynamicCast<xiiStateMachineNode*>(nodes[i]);
    if (pNode != nullptr && pNode->m_bIsInitialState)
    {
      const xiiUuid& nodeUuid = nodeUuids[i];
      XII_SUCCEED_OR_RETURN(AddState(pNode, nodeUuid));
      XII_ASSERT_DEV(nodeUuidToStateIndex[nodeUuid] == 0, "Initial state has to have index 0");
      break;
    }
  }

  if (nodeUuidToStateIndex.IsEmpty())
  {
    return xiiStatus("Initial state is not set");
  }

  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    auto pNode = xiiDynamicCast<xiiStateMachineNode*>(nodes[i]);
    if (pNode == nullptr || pNode->m_bIsInitialState)
      continue;

    XII_SUCCEED_OR_RETURN(AddState(pNode, nodeUuids[i]));
  }

  for (auto pConnection : connections)
  {
    xiiUniquePtr<xiiStateMachineTransition> pTransition = xiiUniquePtr<xiiStateMachineTransition>(pConnection->m_pType, nullptr);
    if (pTransition == nullptr)
    {
      pTransition = XII_DEFAULT_NEW(xiiStateMachineTransition_Timeout);
    }

    xiiUInt32 uiFromStateIndex = xiiInvalidIndex;
    xiiUInt32 uiToStateIndex   = xiiInvalidIndex;
    nodeUuidToStateIndex.TryGetValue(pConnection->m_Source, uiFromStateIndex); // Can fail for any states
    XII_VERIFY(nodeUuidToStateIndex.TryGetValue(pConnection->m_Target, uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  xiiDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // Asset Header
  {
    xiiAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();
  }

  XII_SUCCEED_OR_RETURN(desc.Serialize(file));

  // do the actual file writing
  if (file.Close().Failed())
    return xiiStatus(xiiFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return XII_SUCCESS;
}
