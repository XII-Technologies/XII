#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

// Shared CPU/GPU layout — must match Orchestration/PerFrameUploadData.h exactly.
#include <Shaders/Pipeline/Orchestration/PerFrameUploadData.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPerFrameBufferUploadPass, 1, xiiRTTIDefaultAllocator<xiiPerFrameBufferUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("Name",   m_sName)  ->AddAttributes(new xiiDefaultValueAttribute("PerFrameBufferUploadPass")),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  struct xiiPerFrameBufferUploadPassAutoReg
  {
    xiiPerFrameBufferUploadPassAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiPerFrameBufferUploadPass)); }
  };
  static xiiPerFrameBufferUploadPassAutoReg s_AutoReg;
}

xiiPerFrameBufferUploadPass::xiiPerFrameBufferUploadPass() : xiiRenderPipelinePass("PerFrameBufferUploadPass") {}

xiiPerFrameBufferUploadPass::~xiiPerFrameBufferUploadPass()
{
  m_pCameraBuffer.Clear();
  m_pLightBuffer.Clear();
  m_pGlobalBuffer.Clear();
}

namespace
{
  struct UploadPassData
  {
    xiiRGBufferHandle         hCameraBuffer;
    xiiRGBufferHandle         hLightBuffer;
    xiiRGBufferHandle         hGlobalBuffer;
    xiiPerFrameCameraUploadData Camera;
    xiiPerFrameLightUploadData  Light;
    xiiPerFrameGlobalUploadData Global;
  };
}

void xiiPerFrameBufferUploadPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Lazy-create structured buffers on first use.
  if (!m_pCameraBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiPerFrameCameraUploadData);
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pCameraBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pLightBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiPerFrameLightUploadData);
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pLightBuffer = pDevice->CreateBuffer(desc);
  }
  if (!m_pGlobalBuffer)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize        = sizeof(xiiPerFrameGlobalUploadData);
    desc.m_BufferFlags   = xiiGALBufferUsageFlags::ConstantBuffer;
    desc.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    m_pGlobalBuffer = pDevice->CreateBuffer(desc);
  }

  // Read resolved render scale from the dynamic resolution pass.
  float fScale = 1.0f;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DynamicResolutionScale), fScale);

  xiiUInt32 uiFrameIndex = 0u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_FrameIndex), uiFrameIndex);

  // Fill CPU-side constants from the view's cached matrices.
  const xiiViewData& vd = view.GetData();

  xiiPerFrameCameraUploadData camera;
  camera.ViewProjectionMatrix        = vd.m_ViewProjectionMatrix[0];
  camera.InverseViewProjectionMatrix = vd.m_InverseViewProjectionMatrix[0];
  camera.CameraPositionAndNearPlane  = xiiVec4(vd.m_InverseViewMatrix[0].GetTranslationVector(), vd.m_fNearPlane);
  camera.CameraForwardAndFarPlane    = xiiVec4(-vd.m_ViewMatrix[0].GetRow(2).GetAsVec3(), vd.m_fFarPlane);

  xiiPerFrameLightUploadData light;
  // Sun direction and ambient are populated by a light extraction step upstream.
  // Defaults keep the renderer functional even when no light extract occurs.
  light.MainLightDirectionAndIntensity = xiiVec4(0.0f, -1.0f, 0.0f, 1.0f);
  light.MainLightColor                 = xiiVec4(1.0f, 0.95f, 0.85f, 1.0f);
  light.AmbientLightColor              = xiiVec4(0.1f, 0.1f, 0.15f, 1.0f);
  light.ActiveLightCount               = 0u;

  xiiPerFrameGlobalUploadData global;
  global.FrameIndex   = uiFrameIndex;
  global.DeltaTimeMs  = static_cast<float>(view.GetData().m_fDeltaTime * 1000.0f);
  global.GlobalTime   = 0.0f; // Filled from world clock by caller if needed.
  global.WorldTime    = 0.0f;
  global.RenderScaleJitter = xiiVec4(fScale, 0.0f, 0.0f, 0.0f);

  auto [pData, hPass] = graph.AddPass<UploadPassData>(
    GetName(),
    xiiGALCommandQueueFlags::Graphics,
    [this](UploadPassData& data, xiiRGBuilder& builder)
    {
      data.hCameraBuffer = builder.ImportBuffer("PerFrameCamera", m_pCameraBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hCameraBuffer = builder.WriteBuffer(data.hCameraBuffer, xiiGALResourceStateFlags::CopyDestination);
      data.hLightBuffer  = builder.ImportBuffer("PerFrameLight", m_pLightBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hLightBuffer  = builder.WriteBuffer(data.hLightBuffer, xiiGALResourceStateFlags::CopyDestination);
      data.hGlobalBuffer = builder.ImportBuffer("PerFrameGlobal", m_pGlobalBuffer, xiiGALResourceStateFlags::ConstantBuffer);
      data.hGlobalBuffer = builder.WriteBuffer(data.hGlobalBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [camera, light, global](const UploadPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Per-Frame Buffer Upload");

      xiiGALBuffer* pCam    = context.GetBuffer(data.hCameraBuffer);
      xiiGALBuffer* pLit    = context.GetBuffer(data.hLightBuffer);
      xiiGALBuffer* pGlobal = context.GetBuffer(data.hGlobalBuffer);

      cmd.UpdateBuffer(pCam,    0u, &camera, sizeof(xiiPerFrameCameraUploadData));
      cmd.UpdateBuffer(pLit,    0u, &light,  sizeof(xiiPerFrameLightUploadData));
      cmd.UpdateBuffer(pGlobal, 0u, &global, sizeof(xiiPerFrameGlobalUploadData));

      cmd.PopDebugGroup();
    },
    /*bHasSideEffects=*/true);

  pData->Camera = camera;
  pData->Light  = light;
  pData->Global = global;

  // Publish imported buffer handles so downstream passes can import them by name.
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameCameraBuffer), pData->hCameraBuffer);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameLightBuffer),  pData->hLightBuffer);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameGlobalBuffer), pData->hGlobalBuffer);
}
