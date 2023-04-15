#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory_RandomRotationSpeed, 2, xiiRTTIDefaultAllocator<xiiParticleInitializerFactory_RandomRotationSpeed>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RandomStartAngle", m_bRandomStartAngle),
    XII_MEMBER_PROPERTY("DegreesPerSecond", m_RotationSpeed)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::Degree(90)), new xiiClampValueAttribute(xiiAngle::Degree(0), xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer_RandomRotationSpeed, 1, xiiRTTIDefaultAllocator<xiiParticleInitializer_RandomRotationSpeed>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiRTTI* xiiParticleInitializerFactory_RandomRotationSpeed::GetInitializerType() const
{
  return xiiGetStaticRTTI<xiiParticleInitializer_RandomRotationSpeed>();
}

void xiiParticleInitializerFactory_RandomRotationSpeed::CopyInitializerProperties(xiiParticleInitializer* pInitializer0, bool bFirstTime) const
{
  xiiParticleInitializer_RandomRotationSpeed* pInitializer = static_cast<xiiParticleInitializer_RandomRotationSpeed*>(pInitializer0);

  pInitializer->m_RotationSpeed     = m_RotationSpeed;
  pInitializer->m_bRandomStartAngle = m_bRandomStartAngle;
}

enum class InitializerRandomRotationVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added start offset

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleInitializerFactory_RandomRotationSpeed::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)InitializerRandomRotationVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_RotationSpeed.m_Value;
  inout_stream << m_RotationSpeed.m_fVariance;

  // Version 2
  inout_stream << m_bRandomStartAngle;
}

void xiiParticleInitializerFactory_RandomRotationSpeed::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_RotationSpeed.m_Value;
  inout_stream >> m_RotationSpeed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRandomStartAngle;
  }
}


void xiiParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", xiiProcessingStream::DataType::Half, &m_pStreamRotationSpeed, true);
  CreateStream("RotationOffset", xiiProcessingStream::DataType::Half, &m_pStreamRotationOffset, true);
}

void xiiParticleInitializer_RandomRotationSpeed::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Random Rotation");

  xiiFloat16* pSpeed = m_pStreamRotationSpeed->GetWritableData<xiiFloat16>();

  // speed
  if (m_RotationSpeed.m_Value != xiiAngle::Radian(0))
  {
    xiiRandom& rng = GetRNG();

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float value = (float)rng.DoubleVariance(m_RotationSpeed.m_Value.GetRadian(), m_RotationSpeed.m_fVariance);

      pSpeed[i]       = m_bPositiveSign ? value : -value;
      m_bPositiveSign = !m_bPositiveSign;
    }
  }
  else
  {
    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSpeed[i] = 0;
    }
  }

  // offset
  if (m_bRandomStartAngle)
  {
    xiiFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<xiiFloat16>();

    xiiRandom& rng = GetRNG();

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = (float)rng.DoubleInRange(-xiiMath::Pi<double>(), +xiiMath::Pi<double>());
    }
  }
  else
  {
    xiiFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<xiiFloat16>();

    for (xiiUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = 0;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class xiiParticleInitializerFactory_RandomRotationSpeed_1_2 : public xiiGraphPatch
{
public:
  xiiParticleInitializerFactory_RandomRotationSpeed_1_2() :
    xiiGraphPatch("xiiParticleInitializerFactory_RandomRotationSpeed", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("DegreesPerSecond").IgnoreResult();
  }
};

xiiParticleInitializerFactory_RandomRotationSpeed_1_2 g_xiiParticleInitializerFactory_RandomRotationSpeed_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
