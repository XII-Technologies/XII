#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GraphicsCore/Rasterizer/RasterizerObject.h>

struct xiiMsgTransformChanged;
struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractOccluderData;

class XII_GRAPHICSCORE_DLL xiiOccluderComponentManager final : public xiiComponentManager<class xiiOccluderComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiOccluderComponentManager(xiiWorld* pWorld);
};

class XII_GRAPHICSCORE_DLL xiiOccluderComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiOccluderComponent, xiiComponent, xiiOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBoxReflectionProbeComponent

public:
  xiiOccluderComponent();
  ~xiiOccluderComponent();

  const xiiVec3& GetExtents() const
  {
    return m_vExtents;
  }

  void SetExtents(const xiiVec3& vExtents);

private:
  xiiVec3 m_vExtents = xiiVec3(5.0f);

  mutable xiiSharedPtr<const xiiRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(xiiMsgExtractOccluderData& msg) const;
};
