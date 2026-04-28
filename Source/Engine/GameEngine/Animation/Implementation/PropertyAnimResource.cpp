/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPropertyAnimResource, 1, xiiRTTIDefaultAllocator<xiiPropertyAnimResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPropertyAnimTarget, 1)
XII_ENUM_CONSTANTS(xiiPropertyAnimTarget::Number, xiiPropertyAnimTarget::VectorX, xiiPropertyAnimTarget::VectorY, xiiPropertyAnimTarget::VectorZ, xiiPropertyAnimTarget::VectorW)
XII_ENUM_CONSTANTS(xiiPropertyAnimTarget::RotationX, xiiPropertyAnimTarget::RotationY, xiiPropertyAnimTarget::RotationZ, xiiPropertyAnimTarget::Color)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiPropertyAnimMode, 1)
XII_ENUM_CONSTANTS(xiiPropertyAnimMode::Once, xiiPropertyAnimMode::Loop, xiiPropertyAnimMode::BackAndForth)
XII_END_STATIC_REFLECTED_ENUM;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiPropertyAnimResource);
// clang-format on

xiiPropertyAnimResource::xiiPropertyAnimResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiPropertyAnimResource, xiiPropertyAnimResourceDescriptor)
{
  m_pDescriptor  = XII_DEFAULT_NEW(xiiPropertyAnimResourceDescriptor);
  *m_pDescriptor = descriptor;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDesc xiiPropertyAnimResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  m_pDescriptor = nullptr;

  return res;
}

xiiResourceLoadDesc xiiPropertyAnimResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiPropertyAnimResource::UpdateContent", GetResourceIdOrDescription());

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = XII_DEFAULT_NEW(xiiPropertyAnimResourceDescriptor);
  m_pDescriptor->Load(*Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiPropertyAnimResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU = m_pDescriptor->m_FloatAnimations.GetHeapMemoryUsage() + sizeof(xiiPropertyAnimResourceDescriptor);
  }
}

void xiiPropertyAnimResourceDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8  uiVersion            = 6;
  const xiiUInt8  uiIdentifier         = 0x0A; // dummy to fill the header to 32 Bit
  const xiiUInt16 uiNumFloatAnimations = static_cast<xiiUInt16>(m_FloatAnimations.GetCount());
  const xiiUInt16 uiNumColorAnimations = static_cast<xiiUInt16>(m_ColorAnimations.GetCount());

  XII_ASSERT_DEV(m_AnimationDuration.GetSeconds() > 0, "Animation duration must be positive");

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << m_AnimationDuration;
  inout_stream << uiNumFloatAnimations;

  xiiCurve1D tmpCurve;

  for (xiiUInt32 i = 0; i < uiNumFloatAnimations; ++i)
  {
    inout_stream << m_FloatAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_FloatAnimations[i].m_sComponentType;
    inout_stream << m_FloatAnimations[i].m_sPropertyPath;
    inout_stream << m_FloatAnimations[i].m_Target;

    tmpCurve = m_FloatAnimations[i].m_Curve;
    tmpCurve.SortControlPoints();
    tmpCurve.ApplyTangentModes();
    tmpCurve.ClampTangents();
    tmpCurve.Save(inout_stream);
  }

  xiiColorGradient tmpGradient;
  inout_stream << uiNumColorAnimations;
  for (xiiUInt32 i = 0; i < uiNumColorAnimations; ++i)
  {
    inout_stream << m_ColorAnimations[i].m_sObjectSearchSequence;
    inout_stream << m_ColorAnimations[i].m_sComponentType;
    inout_stream << m_ColorAnimations[i].m_sPropertyPath;
    inout_stream << m_ColorAnimations[i].m_Target;

    tmpGradient = m_ColorAnimations[i].m_Gradient;
    tmpGradient.SortControlPoints();
    tmpGradient.Save(inout_stream);
  }

  // Version 6
  m_EventTrack.Save(inout_stream);
}

void xiiPropertyAnimResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8  uiVersion       = 0;
  xiiUInt8  uiIdentifier    = 0;
  xiiUInt16 uiNumAnimations = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  XII_ASSERT_DEV(uiIdentifier == 0x0A, "File does not contain a valid xiiPropertyAnimResourceDescriptor");
  XII_ASSERT_DEV(uiVersion == 4 || uiVersion == 5 || uiVersion == 6, "Invalid file version {0}", uiVersion);

  inout_stream >> m_AnimationDuration;

  if (uiVersion == 4)
  {
    xiiEnum<xiiPropertyAnimMode> mode;
    inout_stream >> mode;
  }

  inout_stream >> uiNumAnimations;
  m_FloatAnimations.SetCount(uiNumAnimations);

  for (xiiUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_FloatAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Curve.Load(inout_stream);
    anim.m_Curve.SortControlPoints();
    anim.m_Curve.CreateLinearApproximation();

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = xiiRTTI::FindTypeByName(anim.m_sComponentType);
  }

  inout_stream >> uiNumAnimations;
  m_ColorAnimations.SetCount(uiNumAnimations);

  for (xiiUInt32 i = 0; i < uiNumAnimations; ++i)
  {
    auto& anim = m_ColorAnimations[i];

    inout_stream >> anim.m_sObjectSearchSequence;
    inout_stream >> anim.m_sComponentType;
    inout_stream >> anim.m_sPropertyPath;
    inout_stream >> anim.m_Target;
    anim.m_Gradient.Load(inout_stream);

    if (!anim.m_sComponentType.IsEmpty())
      anim.m_pComponentRtti = xiiRTTI::FindTypeByName(anim.m_sComponentType);
  }

  if (uiVersion >= 6)
  {
    m_EventTrack.Load(inout_stream);
  }
}



XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimResource);
