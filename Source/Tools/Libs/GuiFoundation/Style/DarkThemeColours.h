/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <QColor>

/// Defines the color scheme for a dark-themed Qt GUI.
///
/// This struct encapsulates all the relevant QColor values used in a dark theme, corresponding to various QPalette roles.
/// It provides a centralized way to manage and apply consistent styling across the application.
struct XII_GUIFOUNDATION_DLL xiiQtDarkThemeColours
{
  /// \name Base Surfaces.
  ///@{

  QColor m_Window  = QColor(0, 0, 0);    ///< Black application shell, matching the editor splash.
  QColor m_Base    = QColor(16, 16, 16); ///< Background color for input fields and scene graphs.
  QColor m_AltBase = QColor(24, 24, 24); ///< Background for alternating rows in views.
  QColor m_Shadow  = QColor(8, 8, 8);    ///< Shadow color used in UI elements like property grid arrays.

  ///@}

  /// \name Text & Foreground.
  ///@{

  QColor m_WindowText  = QColor(222, 222, 222); ///< Color of text displayed on windows.
  QColor m_Text        = QColor(222, 222, 222); ///< Default text color.
  QColor m_BrightText  = QColor(255, 85, 85);   ///< Bright text used for alerts or emphasis.
  QColor m_ButtonText  = QColor(222, 222, 222); ///< Text color for buttons.
  QColor m_Placeholder = QColor(144, 144, 144); ///< Color for placeholder text in input fields.

  ///@}

  /// \name Buttons & Controls
  ///@{

  QColor m_Button   = QColor(26, 26, 26); ///< Background color for buttons and toolbars.
  QColor m_Light    = QColor(64, 64, 64); ///< Light accent used in gradients and tab lines.
  QColor m_Midlight = QColor(48, 48, 48); ///< Midlight accent used in subtle highlights.
  QColor m_Dark     = QColor(32, 32, 32); ///< Dark accent used for underlines and separators.
  QColor m_Mid      = QColor(42, 42, 42); ///< Mid-tone used for group box outlines and borders.

  ///@}

  /// \name Highlights & Links.
  ///@{

  QColor m_Highlight     = QColor(109, 169, 247); ///< The splash screen's light-blue accent.
  QColor m_HighlightText = QColor(8, 8, 8);       ///< Dark text keeps light-blue selections readable.
  QColor m_Link          = QColor(109, 169, 247); ///< Color for hyperlinks.
  QColor m_LinkVisited   = QColor(173, 177, 230); ///< Color for visited hyperlinks.

  ///@}

  /// \name Tooltips.
  ///@{

  QColor m_ToolTipBase = QColor(26, 26, 26);    ///< Background color of tooltips.
  QColor m_ToolTipText = QColor(222, 222, 222); ///< Text color within tooltips.

  ///@}

  /// \name Disabled State.
  ///@{

  QColor m_DisabledWindowText = QColor(128, 128, 128); ///< Text color for disabled window elements.
  QColor m_DisabledButton     = QColor(20, 20, 20);    ///< Background color for disabled buttons.
  QColor m_DisabledText       = QColor(105, 105, 105); ///< Text color for disabled input fields.
  QColor m_DisabledButtonText = QColor(128, 128, 128); ///< Text color for disabled buttons.
  QColor m_DisabledHighlight  = QColor(62, 72, 88);    ///< Highlight color in disabled state.
  QColor m_DisabledBrightText = QColor(255, 255, 255); ///< Bright text color in disabled state.

  ///@}

  /// \name NoRole Fallback.
  ///@{

  QColor m_NoRole = QColor(0, 0, 0); ///< Fallback color for undefined palette roles.

  ///@}
};
