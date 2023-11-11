#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct xiiMsgUpdateLocalBounds;

using xiiBakedProbesVolumeComponentManager = xiiComponentManager<class xiiBakedProbesVolumeComponent, xiiBlockStorageType::Compact>;

class XII_RENDERERCORE_DLL xiiBakedProbesVolumeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiBakedProbesVolumeComponent, xiiComponent, xiiBakedProbesVolumeComponentManager);

public:
  xiiBakedProbesVolumeComponent();
  ~xiiBakedProbesVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const xiiVec3& GetExtents() const { return m_vExtents; }
  void           SetExtents(const xiiVec3& vExtents);

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

private:
  xiiVec3 m_vExtents = xiiVec3(10.0f);
};
