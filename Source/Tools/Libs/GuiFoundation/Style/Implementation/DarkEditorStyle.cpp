/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <QApplication>
#include <QStyleFactory>
#include <QWidget>

xiiQtDarkEditorStyle::xiiQtDarkEditorStyle(QStyle* pBaseStyle) :
  QProxyStyle(pBaseStyle ? pBaseStyle : QStyleFactory::create("fusion"))
{
  ApplyPalette();
}

void xiiQtDarkEditorStyle::ApplyPalette()
{
  QPalette palette = BuildPaletteFromTheme();

  QApplication::setPalette(palette);
}

QPalette xiiQtDarkEditorStyle::standardPalette() const
{
  return BuildPaletteFromTheme();
}

int xiiQtDarkEditorStyle::pixelMetric(PixelMetric metric, const QStyleOption* pOption, const QWidget* pWidget) const
{
  // Qt scales these logical pixels for each screen. Never multiply by devicePixelRatio here.
  switch (metric)
  {
    case PM_SmallIconSize:
    case PM_ToolBarIconSize:
      return 16;
    case PM_ToolBarItemMargin:
    case PM_ToolBarItemSpacing:
      return 2;
    case PM_ToolBarFrameWidth:
      return 0;
    case PM_DockWidgetSeparatorExtent:
      return 4;
    case PM_ScrollBarExtent:
      return 12;
    default:
      return QProxyStyle::pixelMetric(metric, pOption, pWidget);
  }
}

QSize xiiQtDarkEditorStyle::sizeFromContents(ContentsType type, const QStyleOption* pOption, const QSize& size, const QWidget* pWidget) const
{
  QSize result = QProxyStyle::sizeFromContents(type, pOption, size, pWidget);
  switch (type)
  {
    case CT_PushButton:
    case CT_ToolButton:
    case CT_ComboBox:
    case CT_LineEdit:
    case CT_SpinBox:
      result.setHeight(qMax(result.height(), pWidget ? pWidget->fontMetrics().height() + 10 : 28));
      break;
    case CT_MenuItem:
      result.rheight() += 4;
      break;
    default:
      break;
  }
  return result;
}

void xiiQtDarkEditorStyle::ApplyDockStyle(QWidget* pDockManager)
{
  // Append to ADS's own sheet so platform-specific buttons and auto-hide behavior survive.
  pDockManager->setStyleSheet(pDockManager->styleSheet() + QStringLiteral(R"(
ads--CDockAreaTitleBar { background: palette(window); border-bottom: 1px solid palette(dark); }
ads--CDockWidgetTab { background: palette(window); border: 0; border-top: 2px solid transparent; padding: 4px 8px; }
ads--CDockWidgetTab[activeTab="true"] { background: palette(button); border-top: 2px solid palette(highlight); }
ads--CDockWidgetTab[focused="true"] { background: palette(button); border-top: 2px solid palette(highlight); }
ads--CDockWidgetTab QLabel { color: palette(text); }
ads--CDockWidget { border: 0; background: palette(window); }
ads--CDockSplitter::handle { background: palette(shadow); }
)"));
}

QPalette xiiQtDarkEditorStyle::BuildPaletteFromTheme() const
{
  QPalette palette;

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

  return palette;
}
