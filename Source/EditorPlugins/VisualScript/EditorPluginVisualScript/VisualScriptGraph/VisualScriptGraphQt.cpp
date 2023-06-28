#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
  xiiQtPin::SetPin(pin);

  const xiiVisualScriptPin& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(pin);

  xiiStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    auto scriptDataType = vsPin.GetScriptDataType();
    if (scriptDataType != xiiVisualScriptDataType::TypedPointer)
    {
      sTooltip.Append(": ", xiiVisualScriptDataType::GetName(scriptDataType));
    }
    else
    {
      sTooltip.Append(": ", vsPin.GetDataType()->GetTypeName());
    }

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

bool xiiQtVisualScriptPin::UpdatePinColors(const xiiColorGammaUB* pOverwriteColor)
{
  xiiColorGammaUB overwriteColor;
  const xiiVisualScriptPin& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*GetPin());
  if (vsPin.GetScriptDataType() == xiiVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const xiiVisualScriptNodeManager*>(vsPin.GetParent()->GetDocumentObjectManager());
    auto deductedType = pManager->GetDeductedType(vsPin.GetParent());
    overwriteColor = xiiVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(deductedType);
    pOverwriteColor = &overwriteColor;
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

  return res;
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptConnection::xiiQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptNode::xiiQtVisualScriptNode() = default;

void xiiQtVisualScriptNode::UpdateState()
{
  xiiStringBuilder sTitle;

  auto pType = GetObject()->GetType();
  if (auto pTitleAttribute = pType->GetAttributeByType<xiiTitleAttribute>())
  {
    sTitle = pTitleAttribute->GetTitle();

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
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
      }
    }

    xiiVariant val;
    xiiStringBuilder sVal;
    for (const auto& prop : properties)
    {
      val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

      if (prop->GetSpecificType()->IsDerivedFrom<xiiEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<xiiBitflagsBase>())
      {
        xiiReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<xiiInt64>(), sVal);
        sVal = xiiTranslate(sVal);
      }
      else if (val.IsA<xiiString>())
      {
        sVal = val.Get<xiiString>();
        if (sVal.GetCharacterCount() > 16)
        {
          sVal.Shrink(0, sVal.GetCharacterCount() - 13);
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
    m_pSubtitleLabel->setPlainText(sSubTitle.GetData());
  }
  else
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pType);
    if (pNodeDesc->m_bNeedsDataTypeDeduction)
    {
      auto pManager = static_cast<const xiiVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());
      xiiVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      const char* sSubTitle = deductedType != xiiVisualScriptDataType::Invalid ? xiiVisualScriptDataType::GetName(deductedType) : "Unknown";
      m_pSubtitleLabel->setPlainText(sSubTitle);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

xiiQtVisualScriptNodeScene::xiiQtVisualScriptNodeScene(QObject* parent /*= nullptr*/)
  : xiiQtNodeScene(parent)
{
}

xiiQtVisualScriptNodeScene::~xiiQtVisualScriptNodeScene() = default;

void xiiQtVisualScriptNodeScene::SetDocumentNodeManager(const xiiDocumentNodeManager* pManager)
{
  xiiQtNodeScene::SetDocumentNodeManager(pManager);

  static_cast<const xiiVisualScriptNodeManager*>(pManager)->m_DeductedTypeChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiQtVisualScriptNodeScene::DeductedTypeChangedHandler, this));
}

void xiiQtVisualScriptNodeScene::DeductedTypeChangedHandler(const xiiDocumentObject* pObject)
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
