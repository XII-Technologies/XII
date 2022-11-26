#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;
class xiiParticleEffectAssetDocument;
class QComboBox;
class QToolButton;
class xiiQtPropertyGridWidget;


class xiiQtParticleEffectAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtParticleEffectAssetDocumentWindow(xiiAssetDocument* pDocument);
  ~xiiQtParticleEffectAssetDocumentWindow();

  virtual const char*             GetWindowLayoutGroupName() const override;
  xiiParticleEffectAssetDocument* GetParticleDocument();

private Q_SLOTS:
  void onSystemSelected(int index);
  void onAddSystem(bool);
  void onRemoveSystem(bool);
  void onRenameSystem(bool);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void SendLiveResourcePreview();
  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const xiiParticleEffectAssetEvent& e);
  void UpdateSystemList();
  void SelectSystem(xiiDocumentObject* pObject);

  xiiParticleEffectAssetDocument* m_pAssetDoc;

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget;

  QComboBox*               m_pSystemsCombo            = nullptr;
  QToolButton*             m_pAddSystem               = nullptr;
  QToolButton*             m_pRemoveSystem            = nullptr;
  QToolButton*             m_pRenameSystem            = nullptr;
  xiiQtPropertyGridWidget* m_pPropertyGridSystems     = nullptr;
  xiiQtPropertyGridWidget* m_pPropertyGridEmitter     = nullptr;
  xiiQtPropertyGridWidget* m_pPropertyGridInitializer = nullptr;
  xiiQtPropertyGridWidget* m_pPropertyGridBehavior    = nullptr;
  xiiQtPropertyGridWidget* m_pPropertyGridType        = nullptr;

  xiiString                             m_sSelectedSystem;
  xiiMap<xiiString, xiiDocumentObject*> m_ParticleSystems;
};
