#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct xiiMsgTransformChanged;
struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractVolumes;

using xiiImageDataResourceHandle = xiiTypedResourceHandle<class xiiImageDataResource>;

class XII_PROCGENPLUGIN_DLL xiiProcVolumeComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiProcVolumeComponent, xiiComponent);

public:
  xiiProcVolumeComponent();
  ~xiiProcVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void  SetValue(float fValue);
  float GetValue() const { return m_fValue; }

  void  SetSortOrder(float fOrder);
  float GetSortOrder() const { return m_fSortOrder; }

  void                         SetBlendMode(xiiEnum<xiiProcGenBlendMode> blendMode);
  xiiEnum<xiiProcGenBlendMode> GetBlendMode() const { return m_BlendMode; }

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  void OnTransformChanged(xiiMsgTransformChanged& ref_msg);

  static const xiiEvent<const xiiProcGenInternal::InvalidatedArea&>& GetAreaInvalidatedEvent() { return s_AreaInvalidatedEvent; }

protected:
  float                        m_fValue     = 1.0f;
  float                        m_fSortOrder = 0.0f;
  xiiEnum<xiiProcGenBlendMode> m_BlendMode;

  void InvalidateArea();
  void InvalidateArea(const xiiBoundingBox& area);

  static xiiEvent<const xiiProcGenInternal::InvalidatedArea&> s_AreaInvalidatedEvent;
};

//////////////////////////////////////////////////////////////////////////

using xiiProcVolumeSphereComponentManager = xiiComponentManager<class xiiProcVolumeSphereComponent, xiiBlockStorageType::Compact>;

class XII_PROCGENPLUGIN_DLL xiiProcVolumeSphereComponent : public xiiProcVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProcVolumeSphereComponent, xiiProcVolumeComponent, xiiProcVolumeSphereComponentManager);

public:
  xiiProcVolumeSphereComponent();
  ~xiiProcVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void  SetRadius(float fRadius);

  float GetFadeOutStart() const { return m_fFadeOutStart; }
  void  SetFadeOutStart(float fFadeOutStart);

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const;

protected:
  float m_fRadius       = 5.0f;
  float m_fFadeOutStart = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using xiiProcVolumeBoxComponentManager = xiiComponentManager<class xiiProcVolumeBoxComponent, xiiBlockStorageType::Compact>;

class XII_PROCGENPLUGIN_DLL xiiProcVolumeBoxComponent : public xiiProcVolumeComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProcVolumeBoxComponent, xiiProcVolumeComponent, xiiProcVolumeBoxComponentManager);

public:
  xiiProcVolumeBoxComponent();
  ~xiiProcVolumeBoxComponent();

  const xiiVec3& GetExtents() const { return m_vExtents; }
  void           SetExtents(const xiiVec3& vExtents);

  const xiiVec3& GetFadeOutStart() const { return m_vFadeOutStart; }
  void           SetFadeOutStart(const xiiVec3& vFadeOutStart);

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const;

protected:
  xiiVec3 m_vExtents      = xiiVec3(10.0f);
  xiiVec3 m_vFadeOutStart = xiiVec3(0.5f);
};

//////////////////////////////////////////////////////////////////////////

using xiiProcVolumeImageComponentManager = xiiComponentManager<class xiiProcVolumeImageComponent, xiiBlockStorageType::Compact>;

class XII_PROCGENPLUGIN_DLL xiiProcVolumeImageComponent : public xiiProcVolumeBoxComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProcVolumeImageComponent, xiiProcVolumeBoxComponent, xiiProcVolumeImageComponentManager);

public:
  xiiProcVolumeImageComponent();
  ~xiiProcVolumeImageComponent();

  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

  void OnExtractVolumes(xiiMsgExtractVolumes& ref_msg) const;

  void        SetImageFile(const char* szFile); // [ property ]
  const char* GetImageFile() const;             // [ property ]

  void                       SetImage(const xiiImageDataResourceHandle& hResource);
  xiiImageDataResourceHandle GetImage() const { return m_hImage; }

protected:
  xiiImageDataResourceHandle m_hImage;
};
