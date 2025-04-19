#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

using xiiColorAnimationComponentManager = xiiComponentManagerSimple<class xiiColorAnimationComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Samples a color gradient and sends a xiiMsgSetColor to the object it is attached to
///
/// The color gradient is sampled linearly over time.
/// This can be used to animate the color of a light source or mesh.
class XII_GAMEENGINE_DLL xiiColorAnimationComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiColorAnimationComponent, xiiComponent, xiiColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiColorAnimationComponent
public:
  xiiColorAnimationComponent();

  /// \brief How long it takes to sample the entire color gradient.
  xiiTime m_Duration; // [ property ]

  void                    SetColorGradient(const xiiColorGradientResourceHandle& hResource);               // [ property ]
  XII_ALWAYS_INLINE const xiiColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; } // [ property ]

  /// \brief How the animation should be played and looped.
  xiiEnum<xiiPropertyAnimMode> m_AnimationMode; // [ property ]

  /// \brief How the color should be applied to the target.
  xiiEnum<xiiSetColorMode> m_SetColorMode; // [ property ]

  bool GetApplyRecursive() const;     // [ property ]
  void SetApplyRecursive(bool value); // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  xiiTime                        m_CurAnimTime;
  xiiColorGradientResourceHandle m_hGradient;
};
