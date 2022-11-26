#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>


xiiQtVisualShaderScene::xiiQtVisualShaderScene(QObject* parent) :
  xiiQtNodeScene(parent)
{
}

xiiQtVisualShaderScene::~xiiQtVisualShaderScene() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtVisualShaderPin::xiiQtVisualShaderPin() = default;

void xiiQtVisualShaderPin::SetPin(const xiiPin& pin)
{
  xiiQtPin::SetPin(pin);

  const xiiVisualShaderPin& shaderPin = xiiStaticCast<const xiiVisualShaderPin&>(pin);

  xiiStringBuilder sTooltip;
  if (!shaderPin.GetTooltip().IsEmpty())
  {
    sTooltip = shaderPin.GetTooltip();
  }
  else
  {
    sTooltip = shaderPin.GetName();
  }

  if (!shaderPin.GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", shaderPin.GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());
}

void xiiQtVisualShaderPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QPainterPath p = path();

  const xiiVisualShaderPin* pVsPin = static_cast<const xiiVisualShaderPin*>(GetPin());

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());

  if (pVsPin->GetType() == xiiPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen pen;
      pen.setColor(qRgb(255, 0, 0));
      pen.setWidth(3);
      pen.setCosmetic(true);
      pen.setStyle(Qt::PenStyle::SolidLine);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);

      painter->setPen(pen);

      painter->drawRect(this->path().boundingRect());
      painter->restore();
      return;
    }
  }

  painter->drawPath(p);
  painter->restore();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiQtVisualShaderNode::xiiQtVisualShaderNode() = default;

void xiiQtVisualShaderNode::InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject)
{
  xiiQtNode::InitNode(pManager, pObject);

  if (auto pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType()))
  {
    m_HeaderColor = xiiToQtColor(pDesc->m_Color);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    xiiLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void xiiQtVisualShaderNode::UpdateState()
{
  xiiStringBuilder temp = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("ShaderNode::"))
    temp.Shrink(12, 0);

  m_pLabel->setPlainText(temp.GetData());
}
