/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/InfraredSensorPass.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInfraredSensorPass, 1, xiiRTTIDefaultAllocator<xiiInfraredSensorPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Radiance", m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("UseViewProfile")),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiInfraredSensorPass::Build(xiiRenderPipelinePassContext& context)
{
  Capture capture;
  XII_SUCCEED_OR_RETURN(Prepare(context, capture));
  if (!capture.m_bActive)
    return XII_SUCCESS;
  Inputs inputs;
  inputs.m_Radiance = context.GetInputTexture(m_Radiance);
  if (!IsColorInput(context, inputs.m_Radiance, 4))
    return context.ReportFailure("A calibrated radiance sensor requires an explicit radiance image.");
  return Record(context, capture, inputs, "Shaders/Sensors/InfraredSensor.xiiShader");
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Sensors_InfraredSensorPass);
