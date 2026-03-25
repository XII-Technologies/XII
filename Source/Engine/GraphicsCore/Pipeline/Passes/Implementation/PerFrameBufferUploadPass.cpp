#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>

xiiRenderGraphPerFrameBufferUploadPass::xiiRenderGraphPerFrameBufferUploadPass()
{
  m_PassDescription.m_sPassName       = xiiMakeHashedString("PerFrameBufferUpload");
  m_PassDescription.m_QueueFlags      = xiiGALCommandQueueFlags::Transfer | xiiGALCommandQueueFlags::Graphics;
  m_PassDescription.m_bHasSideEffects = true;

  m_sCameraConstantsResourceName = xiiMakeHashedString("CameraConstants");
  m_sLightDataResourceName       = xiiMakeHashedString("FrameLightData");
  m_sGlobalParamsResourceName    = xiiMakeHashedString("GlobalParams");

  RebuildResourceLayout();
}

void xiiRenderGraphPerFrameBufferUploadPass::SetEnabled(bool bEnabled)
{
  m_bEnabled = bEnabled;
}

void xiiRenderGraphPerFrameBufferUploadPass::SetHasSideEffects(bool bHasSideEffects)
{
  m_PassDescription.m_bHasSideEffects = bHasSideEffects;
}

void xiiRenderGraphPerFrameBufferUploadPass::SetCameraConstantsResourceName(xiiHashedString sResourceName)
{
  m_sCameraConstantsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphPerFrameBufferUploadPass::SetLightDataResourceName(xiiHashedString sResourceName)
{
  m_sLightDataResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphPerFrameBufferUploadPass::SetGlobalParamsResourceName(xiiHashedString sResourceName)
{
  m_sGlobalParamsResourceName = sResourceName;
  RebuildResourceLayout();
}

void xiiRenderGraphPerFrameBufferUploadPass::SetSetupCommandListFunc(SetupCommandListFunc setupCommandListFunc)
{
  m_SetupCommandListFunc = setupCommandListFunc;
}

void xiiRenderGraphPerFrameBufferUploadPass::SetUploadCommandListFunc(UploadCommandListFunc uploadCommandListFunc)
{
  m_UploadCommandListFunc = uploadCommandListFunc;
}

void xiiRenderGraphPerFrameBufferUploadPass::SetPostUploadCommandListFunc(PostUploadCommandListFunc postUploadCommandListFunc)
{
  m_PostUploadCommandListFunc = postUploadCommandListFunc;
}

void xiiRenderGraphPerFrameBufferUploadPass::ClearSetupCommandListFunc()
{
  m_SetupCommandListFunc = {};
}

void xiiRenderGraphPerFrameBufferUploadPass::ClearUploadCommandListFunc()
{
  m_UploadCommandListFunc = {};
}

void xiiRenderGraphPerFrameBufferUploadPass::ClearPostUploadCommandListFunc()
{
  m_PostUploadCommandListFunc = {};
}

const xiiRenderGraphPassDescription& xiiRenderGraphPerFrameBufferUploadPass::GetDescription() const
{
  return m_PassDescription;
}

void xiiRenderGraphPerFrameBufferUploadPass::RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const
{
  if (!m_bEnabled || executionContext.m_pCommandList == nullptr)
  {
    return;
  }

  if (m_SetupCommandListFunc.IsValid())
  {
    m_SetupCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (m_UploadCommandListFunc.IsValid())
  {
    m_UploadCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }

  if (m_PostUploadCommandListFunc.IsValid())
  {
    m_PostUploadCommandListFunc(*executionContext.m_pCommandList, executionContext);
  }
}

void xiiRenderGraphPerFrameBufferUploadPass::RebuildResourceLayout()
{
  m_PassDescription.m_Inputs.Clear();
  m_PassDescription.m_Outputs.Clear();

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sCameraConstantsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::CopyDestination;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sLightDataResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::CopyDestination;
  }

  {
    xiiRenderGraphResourceUsage& output = m_PassDescription.m_Outputs.ExpandAndGetRef();
    output.m_sResourceName              = m_sGlobalParamsResourceName;
    output.m_AccessFlags                = xiiRenderGraphResourceAccessFlags::Write;
    output.m_RequiredState              = xiiGALResourceStateFlags::CopyDestination;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_PerFrameBufferUploadPass);
