/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QLineEdit>

class xiiQtFilePropertyWidget;

/// A QLineEdit that is used by xiiQtFilePropertyWidget
class XII_EDITORFRAMEWORK_DLL xiiQtFileLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  explicit xiiQtFileLineEdit(xiiQtFilePropertyWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  xiiQtFilePropertyWidget* m_pOwner = nullptr;
};
