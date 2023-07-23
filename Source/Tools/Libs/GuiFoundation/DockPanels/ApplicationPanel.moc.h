#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>

class xiiQtContainerWindow;

/// \brief Base class for all panels that are supposed to be application wide (not tied to some document).
class XII_GUIFOUNDATION_DLL xiiQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  xiiQtApplicationPanel(xiiStringView sPanelName);
  ~xiiQtApplicationPanel();

  void EnsureVisible();

  static const xiiDynamicArray<xiiQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);
  virtual bool event(QEvent* event) override;

private:
  friend class xiiQtContainerWindow;

  static xiiDynamicArray<xiiQtApplicationPanel*> s_AllApplicationPanels;

  xiiQtContainerWindow* m_pContainerWindow;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GUIFOUNDATION_DLL, xiiQtApplicationPanel);
