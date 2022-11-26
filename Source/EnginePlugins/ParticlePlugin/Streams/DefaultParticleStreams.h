#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_ZeroInit final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_ZeroInit, xiiParticleStream);

protected:
  // base class implementation already zero fills the stream data
  // virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Position final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Position, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Position();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Position final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Position, xiiParticleStream);

protected:
  virtual void Initialize(xiiParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Size final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Size, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Size();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Size final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Size, xiiParticleStream);

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Color final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Color, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Color();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Color final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Color, xiiParticleStream);

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Velocity final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Velocity, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Velocity();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Velocity final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Velocity, xiiParticleStream);

protected:
  virtual void Initialize(xiiParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// LIFETIME STREAM
//////////////////////////////////////////////////////////////////////////

// always default initialized by the behavior

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_LastPosition final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_LastPosition, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_LastPosition();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_RotationSpeed final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_RotationSpeed, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_RotationSpeed();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_RotationOffset final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_RotationOffset, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_RotationOffset();
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_EffectID final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_EffectID, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_EffectID();
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_OnOff final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_OnOff, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_OnOff();
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Axis final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Axis, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Axis();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Axis final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Axis, xiiParticleStream);

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_TrailData final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_TrailData, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_TrailData();
};

//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////

class XII_PARTICLEPLUGIN_DLL xiiParticleStreamFactory_Variation final : public xiiParticleStreamFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStreamFactory_Variation, xiiParticleStreamFactory);

public:
  xiiParticleStreamFactory_Variation();
};

class XII_PARTICLEPLUGIN_DLL xiiParticleStream_Variation final : public xiiParticleStream
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleStream_Variation, xiiParticleStream);

protected:
  virtual void Initialize(xiiParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiParticleSystemInstance* m_pOwner;
};
