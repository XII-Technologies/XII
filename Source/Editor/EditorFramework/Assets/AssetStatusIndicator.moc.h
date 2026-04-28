/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QFrame>

struct xiiAssetCuratorEvent;
class xiiAssetDocument;
struct xiiDocumentEvent;
class QPushButton;

class XII_EDITORFRAMEWORK_DLL xiiQtAssetStatusIndicator : public QFrame
{
  Q_OBJECT

public:
  xiiQtAssetStatusIndicator(xiiAssetDocument* pDoc, QWidget* pParent = nullptr);
  ~xiiQtAssetStatusIndicator();

private Q_SLOTS:
  void onClick(bool);

private:
  void DocumentEventHandler(const xiiDocumentEvent& e);
  void AssetEventHandler(const xiiAssetCuratorEvent& e);

  void UpdateDisplay();

  enum class Action
  {
    None,
    Save,
    Transform,
    ShowErrors,
  };

  xiiAssetDocument* m_pAsset = nullptr;
  QPushButton*      m_pLabel = nullptr;
  Action            m_Action = Action::None;
};
