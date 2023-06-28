#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

xiiQtVisualScriptAssetScene::xiiQtVisualScriptAssetScene(QObject* parent) :
  xiiQtNodeScene(parent)
{
}

xiiQtVisualScriptAssetScene::~xiiQtVisualScriptAssetScene() = default;

void xiiQtVisualScriptAssetScene::GetAllVsNodes(xiiDynamicArray<const xiiDocumentObject*>& allNodes) const
{
  xiiVisualScriptTypeRegistry* pTypeRegistry = xiiVisualScriptTypeRegistry::GetSingleton();
  const xiiRTTI*               pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();

  allNodes.Clear();
  allNodes.Reserve(64);

  const auto& children = GetDocumentNodeManager()->GetRootObject()->GetChildren();
  for (const xiiDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pNodeBaseRtti))
      continue;

    allNodes.PushBack(pObject);
  }
}

void xiiQtVisualScriptAssetScene::VisualScriptActivityEventHandler(const xiiVisualScriptActivityEvent& ae)
{
  // ignore activity from other objects
  if (ae.m_ObjectGuid != m_DebugObject)
    return;

  const xiiVisualScriptInstanceActivity* pActivity = ae.m_pActivityData;

  const xiiDocumentNodeManager* pNodeManager = GetDocumentNodeManager();

  xiiDynamicArray<const xiiDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  const xiiTime tNow       = xiiTime::Now();
  const xiiTime tHighlight = tNow + xiiTime::Milliseconds(300);

  for (auto act : pActivity->m_ActiveExecutionConnections)
  {
    const xiiUInt32 uiNode = act >> 16;
    const xiiUInt32 uiPin  = act & 0x0000FFFF;

    if (uiNode >= allNodes.GetCount())
      continue;

    const xiiDocumentObject* pSrcObject = allNodes[uiNode];
    xiiQtNode*               pQtNode    = m_Nodes[pSrcObject];

    if (pQtNode == nullptr)
      continue;

    const xiiPin* pFoundPin = nullptr;
    {
      auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

      for (auto& pSearchPin : outputPins)
      {
        if (static_cast<const xiiVisualScriptPin&>(*pSearchPin).GetDescriptor()->m_uiPinIndex == uiPin)
        {
          pFoundPin = pSearchPin.Borrow();
          break;
        }
      }

      if (pFoundPin == nullptr)
        continue;
    }

    xiiQtPin* pQtPin = pQtNode->GetOutputPin(*pFoundPin);

    const xiiArrayPtr<xiiQtConnection*> connectionsOut = pQtPin->GetConnections();

    for (auto pQtCon : connectionsOut)
    {
      xiiQtVisualScriptConnection* pVsCon = static_cast<xiiQtVisualScriptConnection*>(pQtCon);

      pVsCon->m_HighlightUntil = tHighlight;

      if (!pVsCon->m_bExecutionHighlight)
      {
        pVsCon->m_bExecutionHighlight = true;
        pVsCon->update();
      }
    }
  }

  ResetActiveConnections(allNodes);
}


void xiiQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler(xiiReflectedClass* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiGatherObjectsForDebugVisMsgInterDoc>())
  {
    if (m_DebugObject.IsValid())
    {
      xiiGatherObjectsForDebugVisMsgInterDoc* pMessage = static_cast<xiiGatherObjectsForDebugVisMsgInterDoc*>(pMsg);
      pMessage->m_Objects.PushBack(m_DebugObject);
    }
  }
}

void xiiQtVisualScriptAssetScene::SetDebugObject(const xiiUuid& objectGuid)
{
  m_DebugObject = objectGuid;
}

void xiiQtVisualScriptAssetScene::ResetActiveConnections(xiiDynamicArray<const xiiDocumentObject*>& allNodes)
{
  const xiiDocumentNodeManager* pNodeManager = GetDocumentNodeManager();

  const xiiTime tNow         = xiiTime::Now();
  bool          bUpdateAgain = false;

  // reset all connections that are not active anymore
  for (xiiUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
  {
    const xiiDocumentObject* pSrcObject = allNodes[srcNodeIdx];
    xiiQtNode*               pQtNode    = m_Nodes[pSrcObject];

    if (pQtNode == nullptr)
      continue;

    auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (auto& pPin : outputPins)
    {
      xiiQtPin* pQtPin = pQtNode->GetOutputPin(*pPin);

      const xiiArrayPtr<xiiQtConnection*> connectionsOut = pQtPin->GetConnections();

      for (auto pQtCon : connectionsOut)
      {
        xiiQtVisualScriptConnection* pVsCon = static_cast<xiiQtVisualScriptConnection*>(pQtCon);

        if (pVsCon->m_bExecutionHighlight)
        {
          if (pVsCon->m_HighlightUntil <= tNow)
          {
            pVsCon->m_bExecutionHighlight = false;
          }

          bUpdateAgain = true;
          pVsCon->update();
        }
      }
    }
  }

  if (bUpdateAgain)
  {
    QTimer::singleShot(100, this, SLOT(OnUpdateDisplay()));
  }
}

void xiiQtVisualScriptAssetScene::OnUpdateDisplay()
{
  xiiDynamicArray<const xiiDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  ResetActiveConnections(allNodes);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptPin::xiiQtVisualScriptPin() = default;

void xiiQtVisualScriptPin::SetPin(const xiiPin& pin)
{
  xiiQtPin::SetPin(pin);

  const xiiVisualScriptPin& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(pin);

  xiiStringBuilder sTooltip;
  if (!vsPin.GetTooltip().IsEmpty())
  {
    sTooltip = vsPin.GetTooltip();
  }
  else
  {
    sTooltip = vsPin.GetName();

    if (vsPin.GetDescriptor()->m_PinType == xiiVisualScriptPinDescriptor::PinType::Data)
    {
      xiiStringBuilder sDataType;
      if (xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiVisualScriptDataPinType>(), vsPin.GetDescriptor()->m_DataType, sDataType, xiiReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sTooltip.Append(": ", sDataType);
      }
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptConnection::xiiQtVisualScriptConnection(QGraphicsItem* parent /*= 0*/) {}


QPen xiiQtVisualScriptConnection::DeterminePen() const
{
  if (m_bExecutionHighlight)
  {
    QPen pen(QBrush(qRgb(255, 50, 50)), 4, Qt::DashLine);
    return pen;
  }

  return xiiQtConnection::DeterminePen();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptNode::xiiQtVisualScriptNode() = default;

void xiiQtVisualScriptNode::InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject)
{
  xiiQtNode::InitNode(pManager, pObject);

  const xiiVisualScriptNodeDescriptor* pDesc = xiiVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc != nullptr)
  {
    m_HeaderColor = qRgb(pDesc->m_Color.r, pDesc->m_Color.g, pDesc->m_Color.b);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    xiiLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void xiiQtVisualScriptNode::UpdateState()
{
  xiiStringBuilder sTitle;

  const xiiVisualScriptNodeDescriptor* pDesc = xiiVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc == nullptr)
    return;

  if (!pDesc->m_sTitle.IsEmpty())
  {
    xiiStringBuilder temp;

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    sTitle = pDesc->m_sTitle;

    for (const auto& pin : GetInputPins())
    {
      if (pin->HasAnyConnections())
      {
        temp.Set("{", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
      }
    }

    xiiVariant       val;
    xiiStringBuilder sVal;
    for (const auto& prop : properties)
    {
      val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

      if (prop->GetSpecificType()->IsDerivedFrom<xiiEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<xiiBitflagsBase>())
      {
        xiiReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<xiiInt64>(), sVal);
        sVal = xiiTranslate(sVal);
      }
      else if (val.CanConvertTo<xiiString>())
      {
        sVal = val.ConvertTo<xiiString>();
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }
  else
  {
    sTitle = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
    if (sTitle.StartsWith_NoCase("VisualScriptNode::"))
      sTitle.Shrink(18, 0);

    if (sTitle.StartsWith_NoCase("xiiVisualScriptNode_"))
      sTitle.Shrink(19, 0);
  }

  m_pTitleLabel->setPlainText(sTitle.GetData());
}
