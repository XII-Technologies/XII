/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/Rendering/TexturePreviewPass.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  template <typename Context>
  class PipelineSelectionDocument : public Context
  {
  public:
    using Context::CreateViewContext;
    using Context::DestroyViewContext;

  protected:
    // Exercise each production view factory without loading an unrelated asset.
    xiiStatus OnInitialize() override { return XII_SUCCESS; }
  };

  template <typename Context>
  void CheckPreviewPipeline(const xiiSharedPtr<xiiGALTextureView>& target, xiiStringView name, const xiiRenderPipelineResourceHandle& expected)
  {
    PipelineSelectionDocument<Context> document;
    const auto                         initialized = document.Initialize(xiiUuid::MakeStableUuidFromString(name), {}, nullptr, name);
    XII_TEST_STATUS(initialized);
    if (initialized.Failed())
      return;
    XII_SCOPE_EXIT(document.Deinitialize());
    auto* context = document.CreateViewContext();
    if (!XII_TEST_BOOL(context != nullptr))
      return;
    XII_SCOPE_EXIT(document.DestroyViewContext(context));
    // An empty main profile must not select, wrap or prevent an Editor preview.
    xiiPlatformProfile profile;
    profile.AddMissingConfigs();
    profile.GetTypeConfig<xiiRenderPipelineProfileConfig>()->m_sMainRenderPipeline.Clear();
    context->SetRenderPipelineProfile(&profile);
    context->SetupRenderTarget(nullptr, target.Borrow(), 16, 16);
    auto* world = document.GetWorld();
    XII_LOCK(world->GetWriteMarker());
    xiiView* view = nullptr;
    if (!XII_TEST_BOOL(world->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(context->GetViewHandle(), view)))
      return;
    XII_TEST_BOOL(view->GetRenderPipelineResource() == expected);
    // Explicit resource selection is preserved verbatim, including complete pipelines.
    const auto override = xiiTexturePreviewPass::GetPipelineResource(false);
    context->SetRenderPipelineResourceReference(override.GetResourceID());
    context->Redraw(false);
    XII_TEST_BOOL(view->GetRenderPipelineResource() == override);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, AssetPreviewPipelineSelection)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {16, 16};
  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target             = environment.GetDevice()->CreateTexture(description);
  if (!XII_TEST_BOOL(target != nullptr))
    return;
  auto       output = target->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  const auto editor = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(xiiRenderPipelineProfileConfig::s_szEditorPipeline);
  CheckPreviewPipeline<xiiMeshContext>(output, "Mesh", editor);
  CheckPreviewPipeline<xiiAnimatedMeshContext>(output, "Animated Mesh", editor);
  CheckPreviewPipeline<xiiAnimationClipContext>(output, "Animation Clip", editor);
  CheckPreviewPipeline<xiiSkeletonContext>(output, "Skeleton", editor);
  CheckPreviewPipeline<xiiMaterialContext>(output, "Material", editor);
  CheckPreviewPipeline<xiiDecalContext>(output, "Decal", editor);
  CheckPreviewPipeline<xiiTextureContext>(output, "Texture", xiiTexturePreviewPass::GetPipelineResource(false));
  CheckPreviewPipeline<xiiTextureCubeContext>(output, "Texture Cube", xiiTexturePreviewPass::GetPipelineResource(true));
}
