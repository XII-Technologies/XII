/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct xiiMsgSetPlaying;

using xiiPropertyAnimComponentManager = xiiComponentManagerSimple<class xiiPropertyAnimComponent, xiiComponentUpdateType::WhenSimulating>;

/// \brief Animates properties on other objects and components according to the property animation resource
///
/// Notes:
///  - There is no messages to change speed, simply modify the speed property.
class XII_GAMEENGINE_DLL xiiPropertyAnimComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPropertyAnimComponent, xiiComponent, xiiPropertyAnimComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPropertyAnimComponent

public:
  xiiPropertyAnimComponent();
  ~xiiPropertyAnimComponent();

  void                                                   SetPropertyAnim(const xiiPropertyAnimResourceHandle& hResource); // [ property ]
  XII_ALWAYS_INLINE const xiiPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; }              // [ property ]

  /// \brief Sets the animation playback range and resets the playing position to the range start position. Also activates the component if it isn't.
  void PlayAnimationRange(xiiTime rangeLow, xiiTime rangeHigh); // [ scriptable ]

  /// \brief Pauses or resumes animation playback. Does not reset any state.
  void OnMsgSetPlaying(xiiMsgSetPlaying& ref_msg); // [ msg handler ]

  xiiEnum<xiiPropertyAnimMode> m_AnimationMode;      // [ property ]
  xiiTime                      m_RandomOffset;       // [ property ]
  float                        m_fSpeed = 1.0f;      // [ property ]
  xiiTime                      m_AnimationRangeLow;  // [ property ]
  xiiTime                      m_AnimationRangeHigh; // [ property ]
  bool                         m_bPlaying = true;    // [ property ]

protected:
  xiiEventMessageSender<xiiMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]
  xiiEventMessageSender<xiiMsgGenericEvent>        m_EventTrackMsgSender; // [ event ]

  struct Binding
  {
    const xiiAbstractMemberProperty* m_pMemberProperty = nullptr;
    mutable void*                    m_pObject         = nullptr; // needs to be updated in case components / objects get relocated in memory
  };

  struct FloatBinding : public Binding
  {
    const xiiFloatPropertyAnimEntry* m_pAnimation[4] = {nullptr, nullptr, nullptr, nullptr};
  };

  struct ComponentFloatBinding : public FloatBinding
  {
    xiiComponentHandle m_hComponent;
  };

  struct GameObjectBinding : public FloatBinding
  {
    xiiGameObjectHandle m_hObject;
  };

  struct ColorBinding : public Binding
  {
    xiiComponentHandle               m_hComponent;
    const xiiColorPropertyAnimEntry* m_pAnimation = nullptr;
  };

  void    Update();
  void    CreatePropertyBindings();
  void    CreateGameObjectBinding(const xiiFloatPropertyAnimEntry* pAnim, const xiiRTTI* pRtti, void* pObject, const xiiGameObjectHandle& hGameObject);
  void    CreateFloatPropertyBinding(const xiiFloatPropertyAnimEntry* pAnim, const xiiRTTI* pRtti, void* pObject, const xiiComponentHandle& hComponent);
  void    CreateColorPropertyBinding(const xiiColorPropertyAnimEntry* pAnim, const xiiRTTI* pRtti, void* pObject, const xiiComponentHandle& hComponent);
  void    ApplyAnimations(const xiiTime& tDiff);
  void    ApplyFloatAnimation(const FloatBinding& binding, xiiTime lookupTime);
  void    ApplySingleFloatAnimation(const FloatBinding& binding, xiiTime lookupTime);
  void    ApplyColorAnimation(const ColorBinding& binding, xiiTime lookupTime);
  xiiTime ComputeAnimationLookup(xiiTime tDiff);
  void    EvaluateEventTrack(xiiTime startTime, xiiTime endTime);
  void    StartPlayback();

  bool m_bReverse = false;

  xiiTime                                  m_AnimationTime;
  xiiHybridArray<GameObjectBinding, 4>     m_GoFloatBindings;
  xiiHybridArray<ComponentFloatBinding, 4> m_ComponentFloatBindings;
  xiiHybridArray<ColorBinding, 4>          m_ColorBindings;
  xiiPropertyAnimResourceHandle            m_hPropertyAnim;

  // we do not want to recreate the binding when the resource changes at runtime
  // therefore we use a sharedptr to keep the data around as long as necessary
  // otherwise that would lead to weird state, because the animation would be interrupted at some point
  // and then the new animation would start from there
  // e.g. when the position is animated, objects could jump around the level
  // when the animation resource is reloaded
  // instead we go with one animation state until this component is reset entirely
  // that means you need to restart a level to see the updated animation
  xiiSharedPtr<xiiPropertyAnimResourceDescriptor> m_pAnimDesc;
};
