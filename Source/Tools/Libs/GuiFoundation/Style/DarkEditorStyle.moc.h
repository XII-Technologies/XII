#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/Style/DarkThemeColours.h>

#include <QProxyStyle>

/// \brief A QProxyStyle that builds on Fusion and applies our dark theme.
class XII_GUIFOUNDATION_DLL xiiQtDarkEditorStyle : public QProxyStyle
{
  Q_OBJECT

public:
  /// \brief Creates the style, defaulting to Fusion if no base style is passed.
  explicit xiiQtDarkEditorStyle(QStyle* pBaseStyle = nullptr);

  ~xiiQtDarkEditorStyle() override = default;

  /// \brief Install the dark palette globally on the current application.
  void ApplyPalette();

  /// \brief Returns a standard palette constructed from dark theme colours.
  QPalette standardPalette() const override;

private:
  /// \brief Helper to build a palette from the theme colours.
  QPalette BuildPaletteFromTheme() const;

private:
  xiiQtDarkThemeColours m_DarkThemeColours;
};
