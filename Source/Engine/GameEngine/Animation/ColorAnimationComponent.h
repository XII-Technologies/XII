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
/// The color gradient is samples linearly over time. This can be used to animate the color of a light source or mesh.
/// \todo Expose the xiiSetColorMode of the xiiMsgSetColor
/// \todo Add speed parameter
/// \todo Add loop mode (once, back-and-forth, loop)
/// \todo Add option to send message to whole sub-tree (SendMessageRecursive)
/// \todo Add on-finished (loop point) event
class XII_GAMEENGINE_DLL xiiColorAnimationComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiColorAnimationComponent, xiiComponent, xiiColorAnimationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent
public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiColorAnimationComponent
public:
  xiiColorAnimationComponent();

  xiiTime m_Duration; // [ property ]

  void        SetColorGradientFile(const char* szFile); // [ property ]
  const char* GetColorGradientFile() const;             // [ property ]

  void                    SetColorGradient(const xiiColorGradientResourceHandle& hResource);
  XII_ALWAYS_INLINE const xiiColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  xiiEnum<xiiPropertyAnimMode> m_AnimationMode; // [ property ]
  xiiEnum<xiiSetColorMode>     m_SetColorMode;  // [ property ]

  bool GetApplyRecursive() const;     // [ property ]
  void SetApplyRecursive(bool value); // [ property ]

  bool GetRandomStartOffset() const;     // [ property ]
  void SetRandomStartOffset(bool value); // [ property ]

protected:
  void Update();

  xiiTime                        m_CurAnimTime;
  xiiColorGradientResourceHandle m_hGradient;
};
