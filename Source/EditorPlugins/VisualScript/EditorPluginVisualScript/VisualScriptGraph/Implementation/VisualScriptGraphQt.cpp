#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, Factories)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVisualScriptNodeRegistry);
    const xiiRTTI* pBaseType = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    xiiQtNodeScene::GetPinFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptPin>(), [](const xiiRTTI* pRtti)->xiiQtPin* { return new xiiQtVisualScriptPin(); });
    /*xiiQtNodeScene::GetConnectionFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualScriptConnection>(), [](const xiiRTTI* pRtti)->xiiQtConnection* { return new xiiQtVisualScriptConnection(); });    */
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const xiiRTTI* pBaseType = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    xiiQtNodeScene::GetPinFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptPin>());
    //xiiQtNodeScene::GetConnectionFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualScriptConnection>());
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    xiiVisualScriptNodeRegistry* pDummy = xiiVisualScriptNodeRegistry::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptPin::xiiQtVisualScriptPin() = default;

void xiiQtVisualScriptPin::SetPin(const xiiPin& pin)
{
  m_bTranslatePinName = false;

  xiiQtPin::SetPin(pin);

  UpdateTooltip();
}

bool xiiQtVisualScriptPin::UpdatePinColors(const xiiColorGammaUB* pOverwriteColor)
{
  xiiColorGammaUB           overwriteColor;
  const xiiVisualScriptPin& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*GetPin());
  if (vsPin.NeedsTypeDeduction())
  {
    auto pManager     = static_cast<const xiiVisualScriptNodeManager*>(vsPin.GetParent()->GetDocumentObjectManager());
    auto deductedType = pManager->GetDeductedType(vsPin);
    overwriteColor    = xiiVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(deductedType);
    pOverwriteColor   = &overwriteColor;
  }

  bool res = xiiQtPin::UpdatePinColors(pOverwriteColor);

  if (vsPin.IsRequired() && HasAnyConnections() == false)
  {
    QColor requiredColor = xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Red));

    QPen p = pen();
    p.setColor(requiredColor);
    setPen(p);

    m_pLabel->setDefaultTextColor(requiredColor);

    return true;
  }

  UpdateTooltip();

  return res;
}

void xiiQtVisualScriptPin::UpdateTooltip()
{
  const xiiVisualScriptPin& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*GetPin());

  xiiStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    sTooltip.Append(": ", vsPin.GetDataTypeName());

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptConnection::xiiQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptNode::xiiQtVisualScriptNode() = default;

void xiiQtVisualScriptNode::UpdateState()
{
  xiiStringBuilder sTitle;

  auto pManager = static_cast<const xiiVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());
  auto pType    = GetObject()->GetType();

  if (auto pTitleAttribute = pType->GetAttributeByType<xiiTitleAttribute>())
  {
    sTitle = pTitleAttribute->GetTitle();

    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    xiiStringBuilder temp;
    for (const auto& pin : GetInputPins())
    {
      if (pin->HasAnyConnections())
      {
        temp.Set("{", pin->GetPin()->GetName(), "}");
        if (static_cast<const xiiVisualScriptPin*>(pin->GetPin())->GetScriptDataType() == xiiVisualScriptDataType::String)
        {
          sTitle.ReplaceAll(temp, "");
        }
        else
        {
          sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
        }

        temp.Set("{?", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, "");
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
      else if (val.IsA<xiiString>() || val.IsA<xiiHashedString>())
      {
        sVal = val.ConvertTo<xiiString>();

        if (prop->GetAttributeByType<xiiAssetBrowserAttribute>())
        {
          if (xiiConversionUtils::IsStringUuid(sVal))
          {
            const xiiUuid AssetGuid = xiiConversionUtils::ConvertStringToUuid(sVal);

            auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

            if (pAsset)
              sVal = pAsset->m_pAssetInfo->m_Path.GetDataDirRelativePath().GetFileName();
            else
              sVal = "<unknown>";
          }
        }

        sVal.ReplaceAll("\n", " ");
        sVal.ReplaceAll("\t", " ");

        if (sVal.GetCharacterCount() > 23)
        {
          sVal.Shrink(0, sVal.GetCharacterCount() - 21);
          sVal.Append("...");
        }
        sVal.Prepend("\"");
        sVal.Append("\"");
      }
      else if (val.CanConvertTo<xiiString>())
      {
        sVal = val.ConvertTo<xiiString>();
      }
      else
      {
        sVal = "<Invalid>";
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);

      temp.Set("{?", prop->GetPropertyName(), "}");
      if (val == xiiVariant(0))
      {
        sTitle.ReplaceAll(temp, "");
      }
      else
      {
        sTitle.ReplaceAll(temp, sVal);
      }
    }
  }
  else
  {
    sTitle = xiiVisualScriptNodeManager::GetNiceTypeName(GetObject());
  }

  if (const char* szSeparator = sTitle.FindSubString("::"))
  {
    m_pTitleLabel->setPlainText(szSeparator + 2);

    xiiStringBuilder sSubTitle = xiiStringView(sTitle.GetData(), szSeparator);
    sSubTitle.Trim("\"");
    m_pSubtitleLabel->setPlainText(sSubTitle.GetData());
  }
  else
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pType);
    if (pNodeDesc != nullptr && pNodeDesc->NeedsTypeDeduction())
    {
      xiiVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      const char*                   sSubTitle    = deductedType != xiiVisualScriptDataType::Invalid ? xiiVisualScriptDataType::GetName(deductedType) : "Unknown";
      m_pSubtitleLabel->setPlainText(sSubTitle);
    }
  }

  auto pScene = static_cast<xiiQtVisualScriptNodeScene*>(scene());

  if (pManager->IsCoroutine(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetCoroutineIcon());
    m_pIcon->setScale(0.5);
  }
  else if (pManager->IsLoop(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetLoopIcon());
    m_pIcon->setScale(0.5);
  }
  else
  {
    m_pIcon->setPixmap(QPixmap());
  }
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptNodeScene::xiiQtVisualScriptNodeScene(QObject* pParent /*= nullptr*/) :
  xiiQtNodeScene(pParent)
{
  constexpr int iconSize = 32;
  m_CoroutineIcon        = QIcon(":/EditorPluginVisualScript/Coroutine.svg").pixmap(QSize(iconSize, iconSize));
  m_LoopIcon             = QIcon(":/EditorPluginVisualScript/Loop.svg").pixmap(QSize(iconSize, iconSize));
}

xiiQtVisualScriptNodeScene::~xiiQtVisualScriptNodeScene()
{
  if (m_pManager != nullptr)
  {
    static_cast<const xiiVisualScriptNodeManager*>(m_pManager)->m_NodeChangedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtVisualScriptNodeScene::NodeChangedHandler, this));
  }
}

void xiiQtVisualScriptNodeScene::InitScene(const xiiDocumentNodeManager* pManager)
{
  xiiQtNodeScene::InitScene(pManager);

  static_cast<const xiiVisualScriptNodeManager*>(pManager)->m_NodeChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiQtVisualScriptNodeScene::NodeChangedHandler, this));
}

void xiiQtVisualScriptNodeScene::NodeChangedHandler(const xiiDocumentObject* pObject)
{
  auto it = m_Nodes.Find(pObject);
  if (it.IsValid() == false)
    return;

  xiiQtNode* pNode = it.Value();

  pNode->ResetFlags();
  pNode->update();

  auto& inputPins = pNode->GetInputPins();
  for (xiiQtPin* pPin : inputPins)
  {
    if (static_cast<xiiQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }

  auto& outputPins = pNode->GetOutputPins();
  for (xiiQtPin* pPin : outputPins)
  {
    if (static_cast<xiiQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }
}
