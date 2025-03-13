#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ads/DockWidget.h>

class xiiDocument;

class XII_GUIFOUNDATION_DLL xiiQtDocumentPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  xiiQtDocumentPanel(QWidget* pParent, xiiDocument* pDocument);
  ~xiiQtDocumentPanel();

  virtual bool event(QEvent* pEvent) override;

private:
  xiiDocument* m_pDocument = nullptr;
};
