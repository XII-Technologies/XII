#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct xiiMsgSetColor;

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

// Bitmask to allow the user to select what debug rendering the component should do
struct DebugRenderComponentMask
{
  using StorageType = xiiUInt8;

  // the enum names for the bits
  enum Enum
  {
    Box    = XII_BIT(0),
    Sphere = XII_BIT(1),
    Cross  = XII_BIT(2),
    Quad   = XII_BIT(3),
    All    = 0xFF,

    // required enum member; used by xiiBitflags for default initialization
    Default = All
  };

  // this allows the debugger to show us names for a bitmask
  // just try this out by looking at an xiiBitflags variable in a debugger
  struct Bits
  {
    xiiUInt8 Box : 1;
    xiiUInt8 Sphere : 1;
    xiiUInt8 Cross : 1;
    xiiUInt8 Quad : 1;
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_SAMPLEGAMEPLUGIN_DLL, DebugRenderComponentMask);

// use xiiComponentUpdateType::Always for this component to use 'Update' called even inside the editor when it is not simulating
// otherwise we would see the debug render output only when simulating the scene
using DebugRenderComponentManager = xiiComponentManagerSimple<class DebugRenderComponent, xiiComponentUpdateType::Always>;

class XII_SAMPLEGAMEPLUGIN_DLL DebugRenderComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(DebugRenderComponent, xiiComponent, DebugRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // DebugRenderComponent

public:
  DebugRenderComponent();
  ~DebugRenderComponent();

  float    m_fSize = 1.0f;            // [ property ]
  xiiColor m_Color = xiiColor::White; // [ property ]

  void                              SetTexture(const xiiTexture2DResourceHandle& hTexture);
  const xiiTexture2DResourceHandle& GetTexture() const;

  void        SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile(void) const;         // [ property ]

  xiiBitflags<DebugRenderComponentMask> m_RenderTypes; // [ property ]

  void OnSetColor(xiiMsgSetColor& msg); // [ msg handler ]

  void SetRandomColor(); // [ scriptable ]

private:
  void Update();

  xiiTexture2DResourceHandle m_hTexture;
};
