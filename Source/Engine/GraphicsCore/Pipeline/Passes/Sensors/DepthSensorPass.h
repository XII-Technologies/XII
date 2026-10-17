/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorCapture.h>
class XII_GRAPHICSCORE_DLL xiiDepthSensorPass final : public xiiSensorCapture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthSensorPass, xiiSensorCapture);

public:
  xiiDepthSensorPass() : xiiSensorCapture(xiiSensorType::DepthCamera) {}
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
