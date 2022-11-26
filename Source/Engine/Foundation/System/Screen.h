#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

/// \brief Describes the properties of a screen
struct XII_FOUNDATION_DLL xiiScreenInfo
{
  xiiString m_sDisplayName; ///< Some OS provided name for the screen, typically the manufacturer and model name.

  xiiInt32 m_iOffsetX;     ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  xiiInt32 m_iOffsetY;     ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  xiiInt32 m_iResolutionX; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  xiiInt32 m_iResolutionY; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool     m_bIsPrimary;   ///< Whether this is the primary/main screen.
};

/// \brief Provides functionality to detect available monitors
class XII_FOUNDATION_DLL xiiScreen
{
public:
  /// \brief Enumerates all available screens. When it returns XII_SUCCESS, at least one screen has been found.
  static xiiResult EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens);

  /// \brief Prints the available screen information to the provided log.
  static void PrintScreenInfo(const xiiHybridArray<xiiScreenInfo, 2>& screens, xiiLogInterface* pLog = xiiLog::GetThreadLocalLogSystem());
};
