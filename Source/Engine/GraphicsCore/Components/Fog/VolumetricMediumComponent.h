/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Lighting/VolumetricMedium.h>

struct xiiMsgTransformChanged;

class xiiVolumetricMediumComponent;

/// Synchronizes active component transforms after the world transform phase.
class XII_GRAPHICSCORE_DLL xiiVolumetricMediumComponentManager : public xiiComponentManager<xiiVolumetricMediumComponent, xiiBlockStorageType::Compact>
{
public:
  explicit xiiVolumetricMediumComponentManager(xiiWorld* pWorld);

  virtual void Initialize() override;
  void         Update(const xiiWorldModule::UpdateContext& context);
};

/// Authors a bounded participating medium such as smoke, steam, dust, or local fog.
///
/// The component owns a generation-checked entry in xiiVolumetricMediumManager. Its
/// world transform is synchronized with the streamed spatial hierarchy while the
/// component is active; deactivation releases the entry immediately.
class XII_GRAPHICSCORE_DLL xiiVolumetricMediumComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVolumetricMediumComponent, xiiRenderComponent, xiiVolumetricMediumComponentManager);

public:
  xiiVolumetricMediumComponent();
  ~xiiVolumetricMediumComponent();

  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  void                              SetShape(xiiEnum<xiiVolumetricMediumShape> shape);
  xiiEnum<xiiVolumetricMediumShape> GetShape() const;
  void                              SetHalfExtents(xiiVec3 vHalfExtents);
  xiiVec3                           GetHalfExtents() const;
  void                              SetScattering(xiiVec3 vScattering);
  xiiVec3                           GetScattering() const;
  void                              SetAbsorption(xiiVec3 vAbsorption);
  xiiVec3                           GetAbsorption() const;
  void                              SetEmission(xiiVec3 vEmission);
  xiiVec3                           GetEmission() const;
  void                              SetAnisotropy(float fAnisotropy);
  float                             GetAnisotropy() const;
  void                              SetPriority(xiiInt32 iPriority);
  xiiInt32                          GetPriority() const;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  void         OnMsgTransformChanged(xiiMsgTransformChanged& ref_msg);

private:
  friend class xiiVolumetricMediumComponentManager;

  void SynchronizeMedium();
  void ReleaseMedium();

  xiiEnum<xiiVolumetricMediumShape> m_Shape        = xiiVolumetricMediumShape::Box;
  xiiVec3                           m_vHalfExtents = xiiVec3(1.0f);
  xiiVec3                           m_vScattering  = xiiVec3(0.08f);
  xiiVec3                           m_vAbsorption  = xiiVec3(0.02f);
  xiiVec3                           m_vEmission    = xiiVec3::MakeZero();
  float                             m_fAnisotropy  = 0.0f;
  xiiInt32                          m_iPriority    = 0;
  xiiVolumetricMediumHandle         m_hMedium;
  xiiTransform                      m_LastGlobalTransform;
  bool                              m_bSynchronizationDirty = true;
};
