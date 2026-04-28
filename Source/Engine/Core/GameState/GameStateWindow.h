/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/System/Window.h>

/// \brief A window class that expands a little on xiiWindow. Default type used by xiiGameState to create a window.
class XII_CORE_DLL xiiGameStateWindow : public xiiWindow
{
public:
  xiiGameStateWindow(const xiiWindowCreationDescription& windowdesc, xiiDelegate<void()> onClickClose = {});
  ~xiiGameStateWindow();

  void ResetOnClickClose(xiiDelegate<void()> onClickClose);

private:
  virtual void OnResize(const xiiSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  xiiDelegate<void()> m_OnClickClose;
};
