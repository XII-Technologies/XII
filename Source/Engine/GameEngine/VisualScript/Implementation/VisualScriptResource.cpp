#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Messages/EventMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptBasicNodes.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

//////////////////////////////////////////////////////////////////////////
/// xiiVisualScriptResource
//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptResource, 2, xiiRTTIDefaultAllocator<xiiVisualScriptResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiVisualScriptResource);
// clang-format on

xiiVisualScriptResource::xiiVisualScriptResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiVisualScriptResource::~xiiVisualScriptResource() = default;

xiiResourceLoadDesc xiiVisualScriptResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiVisualScriptResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiVisualScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiVisualScriptResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiVisualScriptResource, xiiVisualScriptResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////
/// xiiVisualScriptResourceDescriptor
//////////////////////////////////////////////////////////////////////////

void xiiVisualScriptResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;

  ref_stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion < 7)
    return;

  xiiUInt32 uiNumNodes   = 0;
  xiiUInt32 uiNumExecCon = 0;
  xiiUInt32 uiNumDataCon = 0;
  xiiUInt32 uiNumProps   = 0;

  ref_stream >> uiNumNodes;
  ref_stream >> uiNumExecCon;
  ref_stream >> uiNumDataCon;
  ref_stream >> uiNumProps;

  m_Nodes.SetCount(uiNumNodes);
  m_ExecutionPaths.SetCountUninitialized(uiNumExecCon);
  m_DataPaths.SetCountUninitialized(uiNumDataCon);
  m_Properties.SetCount(uiNumProps);

  xiiStringBuilder sType;
  for (auto& node : m_Nodes)
  {
    ref_stream >> sType;

    node.m_isMsgSender    = 0;
    node.m_isMsgHandler   = 0;
    node.m_isFunctionCall = 0;

    if (sType.EndsWith("<call>"))
    {
      node.m_isFunctionCall = 1;

      // remove the <call> part (leave full class name and function name in m_sTypeName
      sType.Shrink(0, 6);
      node.m_sTypeName = sType;

      const char* szColon = sType.FindLastSubString("::");
      sType.SetSubString_FromTo(sType.GetData(), szColon);

      node.m_pType = xiiRTTI::FindTypeByName(sType);
    }
    else
    {
      if (sType.EndsWith("<send>"))
      {
        sType.Shrink(0, 6);
        node.m_isMsgSender = 1;
      }
      else if (sType.EndsWith("<handle>"))
      {
        sType.Shrink(0, 8);
        node.m_isMsgHandler = 1;
      }

      node.m_sTypeName = sType;
      node.m_pType     = xiiRTTI::FindTypeByName(sType);
    }

    ref_stream >> node.m_uiFirstProperty;
    ref_stream >> node.m_uiNumProperties;
  }

  for (auto& con : m_ExecutionPaths)
  {
    ref_stream >> con.m_uiSourceNode;
    ref_stream >> con.m_uiTargetNode;
    ref_stream >> con.m_uiOutputPin;
    ref_stream >> con.m_uiInputPin;
  }

  for (auto& con : m_DataPaths)
  {
    ref_stream >> con.m_uiSourceNode;
    ref_stream >> con.m_uiTargetNode;
    ref_stream >> con.m_uiOutputPin;
    ref_stream >> con.m_uiOutputPinType;
    ref_stream >> con.m_uiInputPin;
    ref_stream >> con.m_uiInputPinType;
  }

  for (auto& prop : m_Properties)
  {
    ref_stream >> prop.m_sName;
    ref_stream >> prop.m_Value;

    if (uiVersion >= 6)
    {
      ref_stream >> prop.m_iMappingIndex;
    }
  }

  // Version 5
  if (uiVersion >= 5)
  {
    xiiUInt32 num;

    ref_stream >> num;
    m_BoolParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      ref_stream >> m_BoolParameters[i].m_sName;
      ref_stream >> m_BoolParameters[i].m_Value;
    }

    ref_stream >> num;
    m_NumberParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      ref_stream >> m_NumberParameters[i].m_sName;
      ref_stream >> m_NumberParameters[i].m_Value;
    }
  }

  // Version 8
  if (uiVersion >= 8)
  {
    xiiUInt32 num;

    ref_stream >> num;
    m_StringParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      ref_stream >> m_StringParameters[i].m_sName;
      ref_stream >> m_StringParameters[i].m_sValue;
    }
  }

  PrecomputeMessageHandlers();
}

void xiiVisualScriptResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 8;

  ref_stream << uiVersion;

  const xiiUInt32 uiNumNodes   = m_Nodes.GetCount();
  const xiiUInt32 uiNumExecCon = m_ExecutionPaths.GetCount();
  const xiiUInt32 uiNumDataCon = m_DataPaths.GetCount();
  const xiiUInt32 uiNumProps   = m_Properties.GetCount();

  ref_stream << uiNumNodes;
  ref_stream << uiNumExecCon;
  ref_stream << uiNumDataCon;
  ref_stream << uiNumProps;

  xiiStringBuilder sType;

  for (const auto& node : m_Nodes)
  {
    if (node.m_pType != nullptr)
    {
      sType = node.m_pType->GetTypeName();
    }
    else
    {
      sType = node.m_sTypeName;
    }

    if (node.m_isMsgSender)
      sType.Append("<send>");
    else if (node.m_isMsgHandler)
      sType.Append("<handle>");
    else if (node.m_isFunctionCall)
      sType.Append("<call>");

    ref_stream << sType;

    ref_stream << node.m_uiFirstProperty;
    ref_stream << node.m_uiNumProperties;
  }

  for (const auto& con : m_ExecutionPaths)
  {
    ref_stream << con.m_uiSourceNode;
    ref_stream << con.m_uiTargetNode;
    ref_stream << con.m_uiOutputPin;
    ref_stream << con.m_uiInputPin;
  }

  for (const auto& con : m_DataPaths)
  {
    ref_stream << con.m_uiSourceNode;
    ref_stream << con.m_uiTargetNode;
    ref_stream << con.m_uiOutputPin;
    ref_stream << con.m_uiOutputPinType;
    ref_stream << con.m_uiInputPin;
    ref_stream << con.m_uiInputPinType;
  }

  for (const auto& prop : m_Properties)
  {
    ref_stream << prop.m_sName;
    ref_stream << prop.m_Value;

    // Version 6
    ref_stream << prop.m_iMappingIndex;
  }

  // Version 5
  {
    ref_stream << m_BoolParameters.GetCount();
    for (const auto& param : m_BoolParameters)
    {
      ref_stream << param.m_sName;
      ref_stream << param.m_Value;
    }

    ref_stream << m_NumberParameters.GetCount();
    for (const auto& param : m_NumberParameters)
    {
      ref_stream << param.m_sName;
      ref_stream << param.m_Value;
    }
  }

  // Version 8
  {
    ref_stream << m_StringParameters.GetCount();
    for (const auto& param : m_StringParameters)
    {
      ref_stream << param.m_sName;
      ref_stream << param.m_sValue;
    }
  }
}

void xiiVisualScriptResourceDescriptor::PrecomputeMessageHandlers()
{
  for (xiiUInt32 uiNode = 0; uiNode < m_Nodes.GetCount(); ++uiNode)
  {
    auto&          node  = m_Nodes[uiNode];
    const xiiRTTI* pType = node.m_pType;

    if (!pType)
      continue;

    xiiUniquePtr<xiiVisualScriptNode> pNode;

    if (pType->IsDerivedFrom<xiiMessage>() && node.m_isMsgHandler)
    {
      auto pHandler                    = xiiVisualScriptNode_MessageHandler::GetStaticRTTI()->GetAllocator()->Allocate<xiiVisualScriptNode_MessageHandler>();
      pHandler->m_pMessageTypeToHandle = pType;

      pNode = pHandler;
    }
    else if (pType->IsDerivedFrom<xiiVisualScriptNode>())
    {
      pNode = pType->GetAllocator()->Allocate<xiiVisualScriptNode>();

      AssignNodeProperties(*pNode, node);
    }
    else
    {
      continue;
    }

    const xiiInt32 iMsgID = pNode->HandlesMessagesWithID();

    if (iMsgID >= 0)
    {
      m_MessageHandlers.Insert(static_cast<xiiUInt16>(iMsgID), static_cast<xiiUInt16>(uiNode));
    }
  }
}

void xiiVisualScriptResourceDescriptor::AssignNodeProperties(xiiVisualScriptNode& ref_vsNode, const Node& properties) const
{
  for (xiiUInt32 i = 0; i < properties.m_uiNumProperties; ++i)
  {
    const xiiUInt32 uiProp = properties.m_uiFirstProperty + i;
    const auto&     prop   = m_Properties[uiProp];

    xiiAbstractProperty* pAbstract = ref_vsNode.GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
    if (pAbstract->GetCategory() != xiiPropertyCategory::Member)
      continue;

    xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);
    xiiReflectionUtils::SetMemberPropertyValue(pMember, &ref_vsNode, prop.m_Value);
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptResource);
