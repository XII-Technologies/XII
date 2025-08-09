#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/Style/DarkThemeColours.h>

#include <QApplication>
#include <QProxyStyle>
#include <QStyleFactory>

// A QProxyStyle that builds on Fusion and applies our dark theme
class XII_GUIFOUNDATION_DLL xiiQtDarkEditorStyle : public QProxyStyle
{
  Q_OBJECT

public:
  explicit xiiQtDarkEditorStyle(QStyle* pBaseStyle = nullptr);
  ~xiiQtDarkEditorStyle() override = default;

  // Install the QPalette using our colors
  void ApplyPalette();

  // Override key painting methods
  void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;

  void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;

private:
  xiiQtDarkThemeColours m_DarkThemeColours;
};
