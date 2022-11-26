#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_ZeroInit, 1, xiiRTTIDefaultAllocator<xiiParticleStream_ZeroInit>)
XII_END_DYNAMIC_REFLECTED_TYPE;



//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Position, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Position>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Position, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Position>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Position::xiiParticleStreamFactory_Position() :
  xiiParticleStreamFactory("Position", xiiProcessingStream::DataType::Float4, xiiGetStaticRTTI<xiiParticleStream_Position>())
{
}

void xiiParticleStream_Position::Initialize(xiiParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void xiiParticleStream_Position::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiVec4> itData(m_pStream, uiNumElements, uiStartIndex);

  const xiiVec4 defValue = m_pOwner->GetTransform().m_vPosition.GetAsVec4(0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Size, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Size>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Size, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Size>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Size::xiiParticleStreamFactory_Size() :
  xiiParticleStreamFactory("Size", xiiProcessingStream::DataType::Half, xiiGetStaticRTTI<xiiParticleStream_Size>())
{
}

void xiiParticleStream_Size::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiFloat16> itData(m_pStream, uiNumElements, uiStartIndex);

  const float defValue = 1.0f;
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Color, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Color>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Color, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Color>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Color::xiiParticleStreamFactory_Color() :
  xiiParticleStreamFactory("Color", xiiProcessingStream::DataType::Half4, xiiGetStaticRTTI<xiiParticleStream_Color>())
{
}

void xiiParticleStream_Color::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiColorLinear16f> itData(m_pStream, uiNumElements, uiStartIndex);

  const xiiColorLinear16f defValue(1.0f, 1.0f, 1.0f, 1.0f);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Velocity, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Velocity>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Velocity, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Velocity>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Velocity::xiiParticleStreamFactory_Velocity() :
  xiiParticleStreamFactory("Velocity", xiiProcessingStream::DataType::Float3, xiiGetStaticRTTI<xiiParticleStream_Velocity>())
{
}

void xiiParticleStream_Velocity::Initialize(xiiParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void xiiParticleStream_Velocity::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const xiiVec3 startVel = m_pOwner->GetParticleStartVelocity();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = startVel;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_LastPosition, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_LastPosition>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_LastPosition::xiiParticleStreamFactory_LastPosition() :
  xiiParticleStreamFactory("LastPosition", xiiProcessingStream::DataType::Float3, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_RotationSpeed, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_RotationSpeed>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_RotationSpeed::xiiParticleStreamFactory_RotationSpeed() :
  xiiParticleStreamFactory("RotationSpeed", xiiProcessingStream::DataType::Half, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_RotationOffset, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_RotationOffset>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_RotationOffset::xiiParticleStreamFactory_RotationOffset() :
  xiiParticleStreamFactory("RotationOffset", xiiProcessingStream::DataType::Half, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_EffectID, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_EffectID>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_EffectID::xiiParticleStreamFactory_EffectID() :
  xiiParticleStreamFactory("EffectID", xiiProcessingStream::DataType::Int, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_OnOff, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_OnOff>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_OnOff::xiiParticleStreamFactory_OnOff() :
  xiiParticleStreamFactory("OnOff", xiiProcessingStream::DataType::Int, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
  // TODO: smaller data type
  // TODO: "Byte" type results in memory corruptions
}

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Axis, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Axis>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Axis, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Axis>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Axis::xiiParticleStreamFactory_Axis() :
  xiiParticleStreamFactory("Axis", xiiProcessingStream::DataType::Float3, xiiGetStaticRTTI<xiiParticleStream_Axis>())
{
}

void xiiParticleStream_Axis::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const xiiVec3 defValue(1, 0, 0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_TrailData, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_TrailData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_TrailData::xiiParticleStreamFactory_TrailData() :
  xiiParticleStreamFactory("TrailData", xiiProcessingStream::DataType::Short2, xiiGetStaticRTTI<xiiParticleStream_ZeroInit>())
{
}


//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStreamFactory_Variation, 1, xiiRTTIDefaultAllocator<xiiParticleStreamFactory_Variation>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleStream_Variation, 1, xiiRTTIDefaultAllocator<xiiParticleStream_Variation>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleStreamFactory_Variation::xiiParticleStreamFactory_Variation() :
  xiiParticleStreamFactory("Variation", xiiProcessingStream::DataType::Int, xiiGetStaticRTTI<xiiParticleStream_Variation>())
{
}

void xiiParticleStream_Variation::Initialize(xiiParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void xiiParticleStream_Variation::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements)
{
  xiiProcessingStreamIterator<xiiUInt32> itData(m_pStream, uiNumElements, uiStartIndex);

  const xiiVec3 startVel = m_pOwner->GetParticleStartVelocity();

  xiiRandom& rng = m_pOwner->GetOwnerEffect()->GetRNG();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = rng.UInt();
    itData.Advance();
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);
