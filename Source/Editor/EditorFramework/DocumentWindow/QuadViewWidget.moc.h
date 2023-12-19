#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

class xiiAssetDocument;
class xiiQtEngineDocumentWindow;
class QGridLayout;
class xiiQtViewWidgetContainer;
class xiiQtEngineViewWidget;
struct xiiEngineViewConfig;
struct xiiEngineViewPreferences;

class XII_EDITORFRAMEWORK_DLL xiiQtQuadViewWidget : public QWidget
{
  Q_OBJECT
public:
  using ViewFactory = xiiDelegate<xiiQtEngineViewWidget*(xiiQtEngineDocumentWindow*, xiiEngineViewConfig*)>;
  xiiQtQuadViewWidget(xiiAssetDocument* pDocument, xiiQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping);
  ~xiiQtQuadViewWidget();

  const xiiHybridArray<xiiQtViewWidgetContainer*, 4>& GetActiveMainViews() { return m_ActiveMainViews; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

protected:
  void SaveViewConfig(const xiiEngineViewConfig& cfg, xiiEngineViewPreferences& pref) const;
  void LoadViewConfig(xiiEngineViewConfig& cfg, xiiEngineViewPreferences& pref);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

private:
  xiiAssetDocument*          m_pDocument;
  xiiQtEngineDocumentWindow* m_pWindow;
  ViewFactory                m_ViewFactory;
  xiiString                  m_sViewToolBarMapping;

  xiiEngineViewConfig                          m_ViewConfigSingle;
  xiiEngineViewConfig                          m_ViewConfigQuad[4];
  xiiHybridArray<xiiQtViewWidgetContainer*, 4> m_ActiveMainViews;
  QGridLayout*                                 m_pViewLayout;
};
