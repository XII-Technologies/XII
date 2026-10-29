/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/Style/DarkThemeColours.h>

#include <QProxyStyle>

/// A QProxyStyle that builds on Fusion and applies our dark theme.
class XII_GUIFOUNDATION_DLL xiiQtDarkEditorStyle : public QProxyStyle
{
  Q_OBJECT

public:
  /// Creates the style, defaulting to Fusion if no base style is passed.
  explicit xiiQtDarkEditorStyle(QStyle* pBaseStyle = nullptr);

  ~xiiQtDarkEditorStyle() override = default;

  /// Install the dark palette globally on the current application.
  void ApplyPalette();

  /// Returns a standard palette constructed from dark theme colours.
  QPalette standardPalette() const override;

  int   pixelMetric(PixelMetric metric, const QStyleOption* pOption = nullptr, const QWidget* pWidget = nullptr) const override;
  QSize sizeFromContents(ContentsType type, const QStyleOption* pOption, const QSize& size, const QWidget* pWidget = nullptr) const override;

  /// Apply the same dock presentation to container and document dock managers.
  static void ApplyDockStyle(QWidget* pDockManager);

private:
  /// Helper to build a palette from the theme colours.
  QPalette BuildPaletteFromTheme() const;

private:
  xiiQtDarkThemeColours m_DarkThemeColours;
};
