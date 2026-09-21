/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;
class xiiProgress;
struct xiiProgressEvent;

/// A Qt implementation to display the state of a xiiProgress instance.
///
/// Create a single instance of this at application startup and link it to a xiiProgress instance.
/// Whenever the instance's progress state changes, this class will display a simple progress bar.
class XII_GUIFOUNDATION_DLL xiiQtProgressbar
{
public:
  xiiQtProgressbar();
  ~xiiQtProgressbar();

  /// Sets the xiiProgress instance that should be visualized.
  void SetProgressbar(xiiProgress* pProgress);

  bool IsProcessingEvents() const { return m_iNestedProcessEvents > 0; }

private:
  void ProgressbarEventHandler(const xiiProgressEvent& e);

  void EnsureCreated();
  void EnsureDestroyed();

  QProgressDialog* m_pDialog              = nullptr;
  xiiProgress*     m_pProgress            = nullptr;
  xiiInt32         m_iNestedProcessEvents = 0;

  QMetaObject::Connection m_OnDialogDestroyed;
};
