/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorCapture.h>
class XII_GRAPHICSCORE_DLL xiiRgbSensorPass final : public xiiSensorCapture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRgbSensorPass, xiiSensorCapture);

public:
  xiiRgbSensorPass() : xiiSensorCapture(xiiSensorType::RGBCamera) {}
  xiiRenderPipelineTextureInputPin m_Radiance{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
