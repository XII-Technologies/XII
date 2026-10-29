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

  QColor m_Window  = QColor(32, 35, 40); ///< Background color of main windows.
  QColor m_Base    = QColor(23, 26, 31); ///< Background color for input fields and scene graphs.
  QColor m_AltBase = QColor(37, 40, 46); ///< Background for alternating rows in views.
  QColor m_Shadow  = QColor(16, 18, 22); ///< Shadow color used in UI elements like property grid arrays.

  ///@}

  /// \name Text & Foreground.
  ///@{

  QColor m_WindowText  = QColor(220, 220, 220); ///< Color of text displayed on windows.
  QColor m_Text        = QColor(220, 220, 220); ///< Default text color.
  QColor m_BrightText  = QColor(255, 85, 85);   ///< Bright text used for alerts or emphasis.
  QColor m_ButtonText  = QColor(220, 220, 220); ///< Text color for buttons.
  QColor m_Placeholder = QColor(153, 161, 174); ///< Color for placeholder text in input fields.

  ///@}

  /// \name Buttons & Controls
  ///@{

  QColor m_Button   = QColor(43, 47, 54); ///< Background color for buttons and toolbars.
  QColor m_Light    = QColor(64, 70, 80); ///< Light accent used in gradients and tab lines.
  QColor m_Midlight = QColor(55, 61, 70); ///< Midlight accent used in subtle highlights.
  QColor m_Dark     = QColor(28, 31, 37); ///< Dark accent used for underlines and separators.
  QColor m_Mid      = QColor(49, 55, 64); ///< Mid-tone used for group box outlines and borders.

  ///@}

  /// \name Highlights & Links.
  ///@{

  QColor m_Highlight     = QColor(37, 116, 145);  ///< Highlight color used for selections.
  QColor m_HighlightText = QColor(255, 255, 255); ///< Text color when highlighted.
  QColor m_Link          = QColor(107, 187, 209); ///< Color for hyperlinks.
  QColor m_LinkVisited   = QColor(161, 147, 198); ///< Color for visited hyperlinks.

  ///@}

  /// \name Tooltips.
  ///@{

  QColor m_ToolTipBase = QColor(43, 47, 54);    ///< Background color of tooltips.
  QColor m_ToolTipText = QColor(235, 238, 242); ///< Text color within tooltips.

  ///@}

  /// \name Disabled State.
  ///@{

  QColor m_DisabledWindowText = QColor(128, 128, 128); ///< Text color for disabled window elements.
  QColor m_DisabledButton     = QColor(28, 31, 37);    ///< Background color for disabled buttons.
  QColor m_DisabledText       = QColor(105, 105, 105); ///< Text color for disabled input fields.
  QColor m_DisabledButtonText = QColor(128, 128, 128); ///< Text color for disabled buttons.
  QColor m_DisabledHighlight  = QColor(70, 90, 110);   ///< Highlight color in disabled state.
  QColor m_DisabledBrightText = QColor(255, 255, 255); ///< Bright text color in disabled state.

  ///@}

  /// \name NoRole Fallback.
  ///@{

  QColor m_NoRole = QColor(0, 0, 0); ///< Fallback color for undefined palette roles.

  ///@}
};
