/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/RttiTypeStringPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

xiiQtRttiTypeStringPropertyWidget::xiiQtRttiTypeStringPropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select Type");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void xiiQtRttiTypeStringPropertyWidget::OnInit()
{
  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(true);
  connect(m_pMenu, &QMenu::aboutToShow, this, &xiiQtRttiTypeStringPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
  m_pButton->setObjectName("Button");

  connect(&m_TypeMenu, &xiiQtTypeMenu::TypeSelected, this, &xiiQtRttiTypeStringPropertyWidget::OnTypeSelected);
}

void xiiQtRttiTypeStringPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  const xiiString sTypeName = value.ConvertTo<xiiString>();

  const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sTypeName);

  if (pRtti == nullptr)
  {
    m_pButton->setText("Select Type");
    m_pButton->setIcon(QIcon());
    return;
  }

  const xiiCategoryAttribute* pCatA = pRtti->GetAttributeByType<xiiCategoryAttribute>();
  const xiiColorAttribute*    pColA = pRtti->GetAttributeByType<xiiColorAttribute>();

  xiiColor iconColor = xiiColor::MakeZero();

  if (pColA)
  {
    iconColor = pColA->GetColor();
  }
  else if (pCatA && iconColor == xiiColor::MakeZero())
  {
    iconColor = xiiColorScheme::GetCategoryColor(pCatA->GetCategory(), xiiColorScheme::CategoryColorUsage::MenuEntryIcon);
  }

  xiiStringBuilder sIconName;
  sIconName.Set(":/TypeIcons/", sTypeName, ".svg");
  const QIcon actionIcon = xiiQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

  m_pButton->setText(sTypeName.GetData());
  m_pButton->setIcon(actionIcon);
}

void xiiQtRttiTypeStringPropertyWidget::onMenuAboutToShow()
{
  if (m_pMenu->isEmpty())
  {
    const xiiRttiTypeStringAttribute* pTypeAttr = m_pProp->GetAttributeByType<xiiRttiTypeStringAttribute>();
    const xiiRTTI*                    pBaseType = xiiRTTI::FindTypeByName(pTypeAttr->GetBaseType());

    m_TypeMenu.FillMenu(m_pMenu, pBaseType, true, false);
  }
}

void xiiQtRttiTypeStringPropertyWidget::OnTypeSelected(QString sTypeName)
{
  const xiiString typeName = sTypeName.toUtf8().data();

  BroadcastValueChanged(typeName);
}
