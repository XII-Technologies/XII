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

void xiiVisualScriptResourceDescriptor::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;

  stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion >= 4 && uiVersion <= 8, "Incorrect version {0} for visual script", uiVersion);

  if (uiVersion < 7)
    return;

  xiiUInt32 uiNumNodes   = 0;
  xiiUInt32 uiNumExecCon = 0;
  xiiUInt32 uiNumDataCon = 0;
  xiiUInt32 uiNumProps   = 0;

  stream >> uiNumNodes;
  stream >> uiNumExecCon;
  stream >> uiNumDataCon;
  stream >> uiNumProps;

  m_Nodes.SetCount(uiNumNodes);
  m_ExecutionPaths.SetCountUninitialized(uiNumExecCon);
  m_DataPaths.SetCountUninitialized(uiNumDataCon);
  m_Properties.SetCount(uiNumProps);

  xiiStringBuilder sType;
  for (auto& node : m_Nodes)
  {
    stream >> sType;

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

    stream >> node.m_uiFirstProperty;
    stream >> node.m_uiNumProperties;
  }

  for (auto& con : m_ExecutionPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
    stream >> con.m_uiInputPin;
  }

  for (auto& con : m_DataPaths)
  {
    stream >> con.m_uiSourceNode;
    stream >> con.m_uiTargetNode;
    stream >> con.m_uiOutputPin;
    stream >> con.m_uiOutputPinType;
    stream >> con.m_uiInputPin;
    stream >> con.m_uiInputPinType;
  }

  for (auto& prop : m_Properties)
  {
    stream >> prop.m_sName;
    stream >> prop.m_Value;

    if (uiVersion >= 6)
    {
      stream >> prop.m_iMappingIndex;
    }
  }

  // Version 5
  if (uiVersion >= 5)
  {
    xiiUInt32 num;

    stream >> num;
    m_BoolParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      stream >> m_BoolParameters[i].m_sName;
      stream >> m_BoolParameters[i].m_Value;
    }

    stream >> num;
    m_NumberParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      stream >> m_NumberParameters[i].m_sName;
      stream >> m_NumberParameters[i].m_Value;
    }
  }

  // Version 8
  if (uiVersion >= 8)
  {
    xiiUInt32 num;

    stream >> num;
    m_StringParameters.SetCount(num);

    for (xiiUInt32 i = 0; i < num; ++i)
    {
      stream >> m_StringParameters[i].m_sName;
      stream >> m_StringParameters[i].m_sValue;
    }
  }

  PrecomputeMessageHandlers();
}

void xiiVisualScriptResourceDescriptor::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 8;

  stream << uiVersion;

  const xiiUInt32 uiNumNodes   = m_Nodes.GetCount();
  const xiiUInt32 uiNumExecCon = m_ExecutionPaths.GetCount();
  const xiiUInt32 uiNumDataCon = m_DataPaths.GetCount();
  const xiiUInt32 uiNumProps   = m_Properties.GetCount();

  stream << uiNumNodes;
  stream << uiNumExecCon;
  stream << uiNumDataCon;
  stream << uiNumProps;

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

    stream << sType;

    stream << node.m_uiFirstProperty;
    stream << node.m_uiNumProperties;
  }

  for (const auto& con : m_ExecutionPaths)
  {
    stream << con.m_uiSourceNode;
    stream << con.m_uiTargetNode;
    stream << con.m_uiOutputPin;
    stream << con.m_uiInputPin;
  }

  for (const auto& con : m_DataPaths)
  {
    stream << con.m_uiSourceNode;
    stream << con.m_uiTargetNode;
    stream << con.m_uiOutputPin;
    stream << con.m_uiOutputPinType;
    stream << con.m_uiInputPin;
    stream << con.m_uiInputPinType;
  }

  for (const auto& prop : m_Properties)
  {
    stream << prop.m_sName;
    stream << prop.m_Value;

    // Version 6
    stream << prop.m_iMappingIndex;
  }

  // Version 5
  {
    stream << m_BoolParameters.GetCount();
    for (const auto& param : m_BoolParameters)
    {
      stream << param.m_sName;
      stream << param.m_Value;
    }

    stream << m_NumberParameters.GetCount();
    for (const auto& param : m_NumberParameters)
    {
      stream << param.m_sName;
      stream << param.m_Value;
    }
  }

  // Version 8
  {
    stream << m_StringParameters.GetCount();
    for (const auto& param : m_StringParameters)
    {
      stream << param.m_sName;
      stream << param.m_sValue;
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

void xiiVisualScriptResourceDescriptor::AssignNodeProperties(xiiVisualScriptNode& vsNode, const Node& properties) const
{
  for (xiiUInt32 i = 0; i < properties.m_uiNumProperties; ++i)
  {
    const xiiUInt32 uiProp = properties.m_uiFirstProperty + i;
    const auto&     prop   = m_Properties[uiProp];

    xiiAbstractProperty* pAbstract = vsNode.GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
    if (pAbstract->GetCategory() != xiiPropertyCategory::Member)
      continue;

    xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);
    xiiReflectionUtils::SetMemberPropertyValue(pMember, &vsNode, prop.m_Value);
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptResource);
