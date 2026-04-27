#pragma once

#include <Core/World/World.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data for a frame-graph node component.
class XII_GRAPHICSCORE_DLL xiiFrameGraphNodeRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiFrameGraphNodeRenderData, xiiRenderData);

public:
  xiiHashedString m_sPassName;
  xiiInt32        m_iPriority = 0;
  bool            m_bEnabled  = true;
};

using xiiFrameGraphNodeComponentManager = xiiComponentManager<class xiiFrameGraphNodeComponent, xiiBlockStorageType::Compact>;

/// \brief Wraps a render-graph pass node so it can be placed in the world and toggled at runtime.
class XII_GRAPHICSCORE_DLL xiiFrameGraphNodeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiFrameGraphNodeComponent, xiiComponent, xiiFrameGraphNodeComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  xiiFrameGraphNodeComponent();
  ~xiiFrameGraphNodeComponent();

  void          SetPassName(xiiStringView sName); // [ property ]
  xiiStringView GetPassName() const;              // [ property ]

  void     SetPriority(xiiInt32 iPriority);            // [ property ]
  xiiInt32 GetPriority() const { return m_iPriority; } // [ property ]

  void SetEnabled(bool b);                       // [ property ]
  bool GetEnabled() const { return m_bEnabled; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiHashedString m_sPassName;
  xiiInt32        m_iPriority = 0;
  bool            m_bEnabled  = true;
};
