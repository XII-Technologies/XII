/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/LidarSensorPass.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLidarSensorPass, 1, xiiRTTIDefaultAllocator<xiiLidarSensorPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
    XII_MEMBER_PROPERTY("Albedo", m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiLidarSensorPass::Build(xiiRenderPipelinePassContext& context)
{
  Capture capture;
  XII_SUCCEED_OR_RETURN(Prepare(context, capture));
  if (!capture.m_bActive)
    return XII_SUCCESS;
  Inputs inputs;
  inputs.m_Normal = context.GetInputTexture(m_Normal);
  inputs.m_Albedo = context.GetInputTexture(m_Albedo);
  if (!IsColorInput(context, inputs.m_Normal, 2) || !IsColorInput(context, inputs.m_Albedo, 4))
    return context.ReportFailure("LiDAR requires explicit octahedral normals and surface reflectance.");
  return Record(context, capture, inputs, "Shaders/Sensors/LidarSensor.xiiShader");
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Sensors_LidarSensorPass);
