/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/RefCounted.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// An owned immutable setup snapshot exchanged through compiled data pins.
/// Runtime values are never serialized as pipeline properties or document identities.
/// A snapshot belongs to one graph setup; external inputs cannot retag older snapshots.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineFrameData : public xiiReflectedClass, public xiiRefCounted
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineFrameData, xiiReflectedClass);

public:
  xiiUInt64 GetSetupSerial() const { return m_uiSetupSerial; }

protected:
  explicit xiiRenderPipelineFrameData(xiiUInt64 setupSerial) : m_uiSetupSerial(setupSerial) {}

private:
  const xiiUInt64 m_uiSetupSerial;
};
