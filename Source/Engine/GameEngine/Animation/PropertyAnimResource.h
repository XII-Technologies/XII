#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/Types/SharedPtr.h>

struct XII_GAMEENGINE_DLL xiiPropertyAnimTarget
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Number,
    VectorX,
    VectorY,
    VectorZ,
    VectorW,
    RotationX,
    RotationY,
    RotationZ,
    Color,

    Default = Number,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

struct XII_GAMEENGINE_DLL xiiPropertyAnimMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Once,
    Loop,
    BackAndForth,

    Default = Loop,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct XII_GAMEENGINE_DLL xiiPropertyAnimEntry
{
  xiiString                      m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  xiiString                      m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  xiiString                      m_sPropertyPath;
  xiiEnum<xiiPropertyAnimTarget> m_Target;
  const xiiRTTI*                 m_pComponentRtti = nullptr;
};

struct XII_GAMEENGINE_DLL xiiFloatPropertyAnimEntry : public xiiPropertyAnimEntry
{
  xiiCurve1D m_Curve;
};

struct XII_GAMEENGINE_DLL xiiColorPropertyAnimEntry : public xiiPropertyAnimEntry
{
  xiiColorGradient m_Gradient;
};

//////////////////////////////////////////////////////////////////////////

// this class is actually ref counted and used with xiiSharedPtr to allow to work on the same data, even when the resource was reloaded
struct XII_GAMEENGINE_DLL xiiPropertyAnimResourceDescriptor : public xiiRefCounted
{
  xiiTime                                    m_AnimationDuration;
  xiiDynamicArray<xiiFloatPropertyAnimEntry> m_FloatAnimations;
  xiiDynamicArray<xiiColorPropertyAnimEntry> m_ColorAnimations;
  xiiEventTrack                              m_EventTrack;

  void Save(xiiStreamWriter& inout_stream) const;
  void Load(xiiStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

using xiiPropertyAnimResourceHandle = xiiTypedResourceHandle<class xiiPropertyAnimResource>;

class XII_GAMEENGINE_DLL xiiPropertyAnimResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPropertyAnimResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiPropertyAnimResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiPropertyAnimResource, xiiPropertyAnimResourceDescriptor);

public:
  xiiPropertyAnimResource();

  xiiSharedPtr<xiiPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiSharedPtr<xiiPropertyAnimResourceDescriptor> m_pDescriptor;
};
