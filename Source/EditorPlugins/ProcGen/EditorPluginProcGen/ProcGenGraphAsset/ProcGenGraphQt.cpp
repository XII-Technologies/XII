#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>


#include <QMenu>
#include <QPainter>

namespace
{
  static xiiColorGammaUB CategoryColor(const char* szCategory)
  {
    xiiColorScheme::Enum color = xiiColorScheme::Green;
    if (xiiStringUtils::IsEqual(szCategory, "Input"))
      color = xiiColorScheme::Lime;
    else if (xiiStringUtils::IsEqual(szCategory, "Output"))
      color = xiiColorScheme::Cyan;
    else if (xiiStringUtils::IsEqual(szCategory, "Math"))
      color = xiiColorScheme::Blue;

    return xiiColorScheme::DarkUI(color);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

xiiQtProcGenNode::xiiQtProcGenNode() = default;

void xiiQtProcGenNode::InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject)
{
  xiiQtNode::InitNode(pManager, pObject);

  const xiiRTTI* pRtti = pObject->GetType();

  if (const xiiCategoryAttribute* pAttr = pRtti->GetAttributeByType<xiiCategoryAttribute>())
  {
    m_HeaderColor = xiiToQtColor(CategoryColor(pAttr->GetCategory()));
  }
}

void xiiQtProcGenNode::UpdateState()
{
  xiiStringBuilder sTitle;

  const xiiRTTI* pRtti        = GetObject()->GetType();
  auto&          typeAccessor = GetObject()->GetTypeAccessor();

  if (const xiiTitleAttribute* pAttr = pRtti->GetAttributeByType<xiiTitleAttribute>())
  {
    xiiStringBuilder temp;
    xiiStringBuilder temp2;

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    sTitle = pAttr->GetTitle();

    for (const auto& pin : GetInputPins())
    {
      temp.Set("{", pin->GetPin()->GetName(), "}");

      if (pin->HasAnyConnections())
      {
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
      }
      else
      {
        temp2.Set("{Input", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, temp2);
      }
    }

    xiiVariant       val;
    xiiStringBuilder sVal;
    xiiStringBuilder sEnumVal;

    for (const auto& prop : properties)
    {
      if (prop->GetCategory() == xiiPropertyCategory::Set)
      {
        sVal = "{";

        xiiHybridArray<xiiVariant, 16> values;
        typeAccessor.GetValues(prop->GetPropertyName(), values);
        for (auto& setVal : values)
        {
          if (sVal.GetElementCount() > 1)
          {
            sVal.Append(", ");
          }
          sVal.Append(setVal.ConvertTo<xiiString>().GetView());
        }

        sVal.Append("}");
      }
      else
      {
        val = typeAccessor.GetValue(prop->GetPropertyName());

        if (prop->GetSpecificType()->IsDerivedFrom<xiiEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<xiiBitflagsBase>())
        {
          xiiReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<xiiInt64>(), sEnumVal);
          sVal = xiiTranslate(sEnumVal);
        }
        else if (prop->GetSpecificType() == xiiGetStaticRTTI<bool>())
        {
          sVal = val.Get<bool>() ? "[x]" : "[ ]";

          if (xiiStringUtils::IsEqual(prop->GetPropertyName(), "Active"))
          {
            SetActive(val.Get<bool>());
          }
        }
        else if (val.CanConvertTo<xiiString>())
        {
          sVal = val.ConvertTo<xiiString>();
        }
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }
  else
  {
    sTitle = pRtti->GetTypeName();
    if (sTitle.StartsWith_NoCase("xiiProcGen"))
    {
      sTitle.Shrink(9, 0);
    }
  }

  m_pTitleLabel->setPlainText(sTitle.GetData());
}

//////////////////////////////////////////////////////////////////////////

xiiQtProcGenPin::xiiQtProcGenPin()  = default;
xiiQtProcGenPin::~xiiQtProcGenPin() = default;

void xiiQtProcGenPin::ExtendContextMenu(QMenu& menu)
{
  QAction* pAction = new QAction("Debug", &menu);
  pAction->setCheckable(true);
  pAction->setChecked(m_bDebug);
  pAction->connect(pAction, &QAction::triggered, [this](bool bChecked) { SetDebug(bChecked); });

  menu.addAction(pAction);
}

void xiiQtProcGenPin::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_D || event->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void xiiQtProcGenPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  xiiQtPin::paint(painter, option, widget);

  painter->save();
  painter->setPen(QPen(QColor(220, 0, 0), 3.5f, Qt::DotLine));
  painter->setBrush(Qt::NoBrush);

  if (m_bDebug)
  {
    float  pad    = 3.5f;
    QRectF bounds = path().boundingRect().adjusted(-pad, -pad, pad, pad);
    painter->drawEllipse(bounds);
  }

  painter->restore();
}

QRectF xiiQtProcGenPin::boundingRect() const
{
  QRectF bounds = xiiQtPin::boundingRect();
  return bounds.adjusted(-6, -6, 6, 6);
}

void xiiQtProcGenPin::SetDebug(bool bDebug)
{
  if (m_bDebug != bDebug)
  {
    m_bDebug = bDebug;

    auto pScene = static_cast<xiiQtProcGenScene*>(scene());
    pScene->SetDebugPin(bDebug ? this : nullptr);

    update();
  }
}

//////////////////////////////////////////////////////////////////////////

xiiQtProcGenScene::xiiQtProcGenScene(QObject* parent /*= nullptr*/) :
  xiiQtNodeScene(parent)
{
}

xiiQtProcGenScene::~xiiQtProcGenScene() = default;

void xiiQtProcGenScene::SetDebugPin(xiiQtProcGenPin* pDebugPin)
{
  if (m_pDebugPin == pDebugPin || m_bUpdatingDebugPin)
    return;

  if (m_pDebugPin != nullptr)
  {
    // don't recursively call this function, otherwise the resource is written twice
    // once with debug disabled, then with it enabled, and because it is so quick after each other
    // the resource manager may ignore the second update, because the first one is still ongoing
    m_bUpdatingDebugPin = true;
    m_pDebugPin->SetDebug(false);
    m_bUpdatingDebugPin = false;
  }

  m_pDebugPin = pDebugPin;

  if (xiiQtDocumentWindow* window = qobject_cast<xiiQtDocumentWindow*>(parent()))
  {
    auto document = static_cast<xiiProcGenGraphAssetDocument*>(window->GetDocument());
    document->SetDebugPin(pDebugPin != nullptr ? pDebugPin->GetPin() : nullptr);
  }
}

xiiStatus xiiQtProcGenScene::RemoveNode(xiiQtNode* pNode)
{
  auto pins = pNode->GetInputPins();
  pins.PushBackRange(pNode->GetOutputPins());

  for (auto pPin : pins)
  {
    if (pPin == m_pDebugPin)
    {
      m_pDebugPin->SetDebug(false);
    }
  }

  return xiiQtNodeScene::RemoveNode(pNode);
}
