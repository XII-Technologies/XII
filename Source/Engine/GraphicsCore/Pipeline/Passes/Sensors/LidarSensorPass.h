/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorCapture.h>
class XII_GRAPHICSCORE_DLL xiiLidarSensorPass final : public xiiSensorCapture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLidarSensorPass, xiiSensorCapture);

public:
  xiiLidarSensorPass() : xiiSensorCapture(xiiSensorType::LiDAR) {}
  xiiRenderPipelineTextureInputPin m_Normal{xiiGALResourceFormat::RG16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureInputPin m_Albedo{xiiGALResourceFormat::Unknown, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
