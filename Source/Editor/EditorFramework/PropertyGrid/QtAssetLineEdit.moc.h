/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

#include <QLineEdit>
#include <QModelIndex>

class xiiQtAssetPropertyWidget;

/// A QLineEdit that is used by xiiQtAssetPropertyWidget
class XII_EDITORFRAMEWORK_DLL xiiQtAssetLineEdit : public QLineEdit
{
  Q_OBJECT

Q_SIGNALS:
  void OpenAsset();
  void SelectAsset();

public:
  explicit xiiQtAssetLineEdit(QWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;

  xiiQtAssetPropertyWidget* m_pOwner = nullptr;
};
