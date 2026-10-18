/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthSensorPass, 1, xiiRTTIDefaultAllocator<xiiDepthSensorPass>)
{
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Sensors") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDepthSensorPass::Build(xiiRenderPipelinePassContext& context)
{
  Capture capture;
  XII_SUCCEED_OR_RETURN(Prepare(context, capture));
  if (!capture.m_bActive)
    return XII_SUCCESS;
  Inputs inputs;
  return Record(context, capture, inputs, "Shaders/Sensors/DepthSensor.xiiShader");
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Sensors_DepthSensorPass);
