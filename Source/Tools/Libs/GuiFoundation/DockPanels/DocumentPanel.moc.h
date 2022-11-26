#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDockWidget>

class xiiDocument;

class XII_GUIFOUNDATION_DLL xiiQtDocumentPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  xiiQtDocumentPanel(QWidget* parent, xiiDocument* pDocument);
  ~xiiQtDocumentPanel();

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;
  virtual bool event(QEvent* event) override;

  static const xiiDynamicArray<xiiQtDocumentPanel*>& GetAllDocumentPanels() { return s_AllDocumentPanels; }

private:
  xiiDocument* m_pDocument = nullptr;

  static xiiDynamicArray<xiiQtDocumentPanel*> s_AllDocumentPanels;
};
