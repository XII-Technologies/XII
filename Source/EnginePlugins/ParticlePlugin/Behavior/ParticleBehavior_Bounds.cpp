#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Bounds.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_Bounds, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_Bounds>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    XII_MEMBER_PROPERTY("BoxExtents", m_vBoxExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(2, 2, 2))),
    XII_ENUM_MEMBER_PROPERTY("OutOfBoundsMode", xiiParticleOutOfBoundsMode, m_OutOfBoundsMode),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiBoxVisualizerAttribute("BoxExtents", 1.0f, xiiColor::LightGreen, nullptr, xiiVisualizerAnchor::Center, xiiVec3::OneVector(), "PositionOffset")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_Bounds, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_Bounds>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleBehaviorFactory_Bounds::xiiParticleBehaviorFactory_Bounds() {}

const xiiRTTI* xiiParticleBehaviorFactory_Bounds::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_Bounds>();
}

void xiiParticleBehaviorFactory_Bounds::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_Bounds* pBehavior = static_cast<xiiParticleBehavior_Bounds*>(pObject);

  pBehavior->m_vPositionOffset = m_vPositionOffset;
  pBehavior->m_vBoxExtents     = m_vBoxExtents;
  pBehavior->m_OutOfBoundsMode = m_OutOfBoundsMode;
}

enum class BehaviorBoundsVersion
{
  Version_0 = 0,
  Version_1, // added out of bounds mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleBehaviorFactory_Bounds::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)BehaviorBoundsVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_vPositionOffset;
  inout_stream << m_vBoxExtents;

  // version 1
  inout_stream << m_OutOfBoundsMode;
}

void xiiParticleBehaviorFactory_Bounds::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)BehaviorBoundsVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_vPositionOffset;
  inout_stream >> m_vBoxExtents;

  if (uiVersion >= 1)
  {
    inout_stream >> m_OutOfBoundsMode;
  }
}

void xiiParticleBehavior_Bounds::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void xiiParticleBehavior_Bounds::QueryOptionalStreams()
{
  m_pStreamLastPosition = GetOwnerSystem()->QueryStream("LastPosition", xiiProcessingStream::DataType::Float3);
}

void xiiParticleBehavior_Bounds::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Bounds");

  const xiiSimdTransform trans    = xiiSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());
  const xiiSimdTransform invTrans = trans.GetInverse();

  const xiiSimdVec4f boxCenter  = xiiSimdConversion::ToVec3(m_vPositionOffset);
  const xiiSimdVec4f boxExt     = xiiSimdConversion::ToVec3(m_vBoxExtents);
  const xiiSimdVec4f halfExtPos = xiiSimdConversion::ToVec3(m_vBoxExtents) * 0.5f;
  const xiiSimdVec4f halfExtNeg = -halfExtPos;

  xiiProcessingStreamIterator<xiiSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  if (m_OutOfBoundsMode == xiiParticleOutOfBoundsMode::Teleport)
  {
    xiiVec3* pLastPosition = nullptr;

    if (m_pStreamLastPosition)
    {
      pLastPosition = m_pStreamLastPosition->GetWritableData<xiiVec3>();
    }

    while (!itPosition.HasReachedEnd())
    {
      const xiiSimdVec4f globalPosCur = itPosition.Current();
      const xiiSimdVec4f localPosCur  = invTrans.TransformPosition(globalPosCur) - boxCenter;

      const xiiSimdVec4f localPosAdd = localPosCur + boxExt;
      const xiiSimdVec4f localPosSub = localPosCur - boxExt;

      xiiSimdVec4f localPosNew;
      localPosNew = xiiSimdVec4f::Select(localPosCur > halfExtPos, localPosSub, localPosCur);
      localPosNew = xiiSimdVec4f::Select(localPosCur < halfExtNeg, localPosAdd, localPosNew);

      localPosNew += boxCenter;
      const xiiSimdVec4f globalPosNew = trans.TransformPosition(localPosNew);

      if (m_pStreamLastPosition)
      {
        const xiiSimdVec4f posDiff = globalPosNew - globalPosCur;
        *pLastPosition += xiiSimdConversion::ToVec3(posDiff);
        ++pLastPosition;
      }

      itPosition.Current() = globalPosNew;
      itPosition.Advance();
    }
  }
  else
  {
    xiiUInt32 idx = 0;

    while (!itPosition.HasReachedEnd())
    {
      const xiiSimdVec4f globalPosCur = itPosition.Current();
      const xiiSimdVec4f localPosCur  = invTrans.TransformPosition(globalPosCur) - boxCenter;

      if ((localPosCur > halfExtPos).AnySet() || (localPosCur < halfExtNeg).AnySet())
      {
        m_pStreamGroup->RemoveElement(idx);
      }

      ++idx;
      itPosition.Advance();
    }
  }
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Bounds);
