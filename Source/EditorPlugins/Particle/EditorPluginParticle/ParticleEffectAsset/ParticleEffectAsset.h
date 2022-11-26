#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Communication/Event.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

class xiiParticleEffectAssetDocument;
struct xiiPropertyMetaStateEvent;

struct xiiParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
    AutoRestartChanged,
    SimulationSpeedChanged,
    RenderVisualizersChanged,
  };

  xiiParticleEffectAssetDocument* m_pDocument;
  Type                            m_Type;
};

class xiiParticleEffectAssetDocument : public xiiSimpleAssetDocument<xiiParticleEffectDescriptor>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEffectAssetDocument, xiiSimpleAssetDocument<xiiParticleEffectDescriptor>);

public:
  xiiParticleEffectAssetDocument(const char* szDocumentPath);

  static void PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e);

  void WriteResource(xiiStreamWriter& stream) const;

  void TriggerRestartEffect();

  xiiEvent<const xiiParticleEffectAssetEvent&> m_Events;

  void SetAutoRestart(bool enable);
  bool GetAutoRestart() const { return m_bAutoRestart; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void  SetSimulationSpeed(float speed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }

  bool GetRenderVisualizers() const { return m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  // Overridden to enable support for visualizers/manipulators
  virtual xiiResult ComputeObjectTransformation(const xiiDocumentObject* pObject, xiiTransform& out_Result) const override;

protected:
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual xiiTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  bool  m_bSimulationPaused  = false;
  bool  m_bAutoRestart       = true;
  bool  m_bRenderVisualizers = false;
  float m_fSimulationSpeed   = 1.0f;
};
