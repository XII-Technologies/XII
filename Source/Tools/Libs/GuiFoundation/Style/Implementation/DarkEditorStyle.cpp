#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Style/DarkEditorStyle.moc.h>

#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionTab>

xiiQtDarkEditorStyle::xiiQtDarkEditorStyle(QStyle* pBaseStyle) :
  QProxyStyle(pBaseStyle ? pBaseStyle : QStyleFactory::create("fusion"))
{
  ApplyPalette();
}

void xiiQtDarkEditorStyle::ApplyPalette()
{
  QPalette palette = proxy()->standardPalette();

  // Base surfaces
  palette.setColor(QPalette::Window, m_DarkThemeColours.m_Window);
  palette.setColor(QPalette::Base, m_DarkThemeColours.m_Base);
  palette.setColor(QPalette::AlternateBase, m_DarkThemeColours.m_AltBase);
  palette.setColor(QPalette::Shadow, m_DarkThemeColours.m_Shadow);

  // Text & foreground
  palette.setColor(QPalette::WindowText, m_DarkThemeColours.m_WindowText);
  palette.setColor(QPalette::Text, m_DarkThemeColours.m_Text);
  palette.setColor(QPalette::BrightText, m_DarkThemeColours.m_BrightText);
  palette.setColor(QPalette::ButtonText, m_DarkThemeColours.m_ButtonText);
  palette.setColor(QPalette::PlaceholderText, m_DarkThemeColours.m_Placeholder);

  // Buttons & controls
  palette.setColor(QPalette::Button, m_DarkThemeColours.m_Button);
  palette.setColor(QPalette::Light, m_DarkThemeColours.m_Light);
  palette.setColor(QPalette::Midlight, m_DarkThemeColours.m_Midlight);
  palette.setColor(QPalette::Dark, m_DarkThemeColours.m_Dark);
  palette.setColor(QPalette::Mid, m_DarkThemeColours.m_Mid);

  // Highlights & links
  palette.setColor(QPalette::Highlight, m_DarkThemeColours.m_Highlight);
  palette.setColor(QPalette::HighlightedText, m_DarkThemeColours.m_HighlightText);
  palette.setColor(QPalette::Link, m_DarkThemeColours.m_Link);
  palette.setColor(QPalette::LinkVisited, m_DarkThemeColours.m_LinkVisited);

  // Tooltips
  palette.setColor(QPalette::ToolTipBase, m_DarkThemeColours.m_ToolTipBase);
  palette.setColor(QPalette::ToolTipText, m_DarkThemeColours.m_ToolTipText);

  // Disabled state
  palette.setColor(QPalette::Disabled, QPalette::WindowText, m_DarkThemeColours.m_DisabledWindowText);
  palette.setColor(QPalette::Disabled, QPalette::Button, m_DarkThemeColours.m_DisabledButton);
  palette.setColor(QPalette::Disabled, QPalette::Text, m_DarkThemeColours.m_DisabledText);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, m_DarkThemeColours.m_DisabledButtonText);
  palette.setColor(QPalette::Disabled, QPalette::Highlight, m_DarkThemeColours.m_DisabledHighlight);
  palette.setColor(QPalette::Disabled, QPalette::BrightText, m_DarkThemeColours.m_DisabledBrightText);

  // NoRole fallback
  palette.setBrush(QPalette::NoRole, QBrush(m_DarkThemeColours.m_NoRole, Qt::NoBrush));

  qApp->setPalette(palette);
}

void xiiQtDarkEditorStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
  QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void xiiQtDarkEditorStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
  QProxyStyle::drawControl(element, option, painter, widget);
}
