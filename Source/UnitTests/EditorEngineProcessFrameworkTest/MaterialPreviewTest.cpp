/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Material/MaterialShaderBinding.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#include <Texture/Image/Image.h>
#include <Texture/Utilities/TextureAssetFile.h>

namespace
{
  class MaterialPreviewTestContext : public xiiMaterialContext
  {
  public:
    using xiiMaterialContext::CreateViewContext;
    using xiiMaterialContext::DestroyViewContext;
    using xiiMaterialContext::UpdateThumbnailViewContext;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialPreview)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!XII_TEST_BOOL(xiiLoadPipelineLibraryResource("Editor").IsValid()))
    return;
  const auto       guid = xiiUuid::MakeStableUuidFromString("MaterialPreview.NativeSurface");
  xiiStringBuilder identity;
  xiiConversionUtils::ToString(guid, identity);
  xiiMaterialResourceDescriptor description;
  description.m_BaseColor     = xiiColor(0.18f, 0.18f, 0.18f);
  const auto         material = xiiResourceManager::CreateResource<xiiMaterialResource>(identity, std::move(description));
  xiiPlatformProfile profile;
  profile.AddMissingConfigs();
  MaterialPreviewTestContext document;
  const auto                 initialized = document.Initialize(guid, {}, nullptr, "Material");
  XII_TEST_STATUS(initialized);
  if (initialized.Failed())
    return;
  XII_SCOPE_EXIT(document.Deinitialize());
  auto* context = document.CreateViewContext();
  if (!XII_TEST_BOOL(context != nullptr))
    return;
  XII_SCOPE_EXIT(document.DestroyViewContext(context));
  context->SetRenderPipelineProfile(&profile);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {256, 256};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto output        = device->CreateTexture(target);
  if (!XII_TEST_BOOL(output != nullptr))
    return;
  context->SetupRenderTarget(nullptr, output->GetDefaultView(xiiGALTextureViewType::RenderTarget), 256, 256);
  document.UpdateThumbnailViewContext(context);
  auto* world = document.GetWorld();
  XII_LOCK(world->GetWriteMarker());
  world->SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  if (!XII_TEST_BOOL(world->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(context->GetViewHandle(), view)))
    return;
  xiiUInt32  captureIndex = 0;
  const auto render       = [&](const xiiPreviewLightingSettings& settings, xiiUInt32& pixel) {
    xiiSimpleDocumentConfigMsgToEngine lighting;
    lighting.m_sWhatToDo    = "PreviewLighting";
    lighting.m_PayloadValue = settings.ToVariant();
    document.HandleMessage(&lighting);
    for (xiiUInt32 frame = 0; frame < 4; ++frame)
    {
      context->Redraw(false);
      world->Update();
      device->WaitIdle();
    }
    XII_TEST_RESULT(view->GetLastRenderResult());
    XII_TEST_INT(world->GetObjectCount(), 3);
    if (!xiiGpuTestReadback::ReadTexturePixel(*device, output.Borrow(), 128, 128, pixel))
      return false;
    xiiDynamicArray<xiiColorGammaUB> pixels;
    if (!xiiGpuTestReadback::ReadTexturePixels(*device, output, pixels))
      return false;
    xiiImage image;
    image.ResetAndAlloc(output->GetDescription());
    xiiMemoryUtils::Copy(image.GetPixelPointer<xiiColorGammaUB>(), pixels.GetData(), pixels.GetCount());
    xiiStringBuilder path(":imgout/MaterialPreviewImages");
    path.AppendFormat("/Preview{}.png", captureIndex++);
    return XII_TEST_RESULT(image.SaveTo(path));
  };
  xiiUInt32 neutral = 0, low = 0, brighter = 0;
  auto      settings = xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::NeutralStudio);
  if (!render(settings, neutral))
    return;
  for (xiiUInt32 channel = 0; channel < 3; ++channel)
  {
    const auto value = (neutral >> (channel * 8)) & 255;
    XII_TEST_BOOL_MSG(value >= 60 && value <= 220, "A lit middle-gray material must be visible without clipping (channel %u = %u).", channel, value);
  }
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::LowLightStudio), low))
    return;
  for (xiiUInt32 channel = 0; channel < 3; ++channel)
    XII_TEST_BOOL_MSG(xiiMath::Abs(static_cast<xiiInt32>((neutral >> (channel * 8)) & 255) - static_cast<xiiInt32>((low >> (channel * 8)) & 255)) < 8, "Equivalent physical light/exposure ratios should remain visually comparable.");
  settings.m_fExposureCompensation = 1;
  if (!render(settings, brighter))
    return;
  XII_TEST_BOOL((brighter & 255) > (neutral & 255));
  for (xiiUInt32 model = 0; model < 3; ++model)
  {
    xiiDocumentConfigMsgToEngine scene;
    scene.m_sWhatToDo = "PreviewModel";
    scene.m_iValue    = model;
    document.HandleMessage(&scene);
    xiiUInt32 pixel = 0;
    if (!render(settings, pixel))
      return;
    XII_TEST_BOOL((pixel & 0x00FFFFFFU) != 0);
  }
  xiiUInt32 outdoor = 0;
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::OutdoorDaylight), outdoor))
    return;
  XII_TEST_BOOL((outdoor & 0x00FFFFFFU) != 0);
  view->SetViewRenderMode(xiiViewRenderMode::BaseColor);
  xiiUInt32 diagnostic = 0;
  if (!render(xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::Diagnostic), diagnostic))
    return;
  XII_TEST_BOOL((diagnostic & 0x00FFFFFFU) != 0);
}

enum class AuthoredPreview
{
  Handwritten,
  Graph,
  GraphTransparent,
  DefaultShader,
  Transparent,
  NativeTransparent,
  Masked,
  Mixed
};
static void TestAuthoredPreview(AuthoredPreview type)
{
  const bool               graph       = type == AuthoredPreview::Graph || type == AuthoredPreview::GraphTransparent;
  const bool               native      = type == AuthoredPreview::NativeTransparent;
  const bool               transparent = type == AuthoredPreview::Transparent || type == AuthoredPreview::GraphTransparent || native;
  const bool               masked      = type == AuthoredPreview::Masked;
  const bool               mixed       = type == AuthoredPreview::Mixed;
  const bool               builtin     = type == AuthoredPreview::DefaultShader;
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!XII_TEST_BOOL(xiiLoadPipelineLibraryResource("Editor").IsValid()))
    return;
  const auto       guid = xiiUuid::MakeStableUuidFromString(graph ? "MaterialPreview.GraphSurface" : masked ? "MaterialPreview.MaskedSurface" :
                                                              mixed                                         ? "MaterialPreview.MixedSurface" :
                                                              transparent                                   ? native ? "MaterialPreview.NativeTransparentSurface" : "MaterialPreview.TransparentSurface" :
                                                                                                              "MaterialPreview.AuthoredSurface");
  xiiStringBuilder identity;
  xiiConversionUtils::ToString(guid, identity);
  const xiiStringBuilder texturePath(":imgout/", identity, ".xiiBinTexture2D");
  const auto             writeTexture = [&](xiiUInt8 value) {
    const xiiUInt8                   texels[] = {value, value, value, 255};
    xiiGALTextureCreationDescription imageDescription;
    imageDescription.m_Size   = {1, 1};
    imageDescription.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    xiiImageView image;
    image.ResetAndViewExternalStorage(imageDescription, xiiConstByteBlobPtr(texels, sizeof(texels)));
    xiiFileWriter file;
    XII_SUCCEED_OR_RETURN(file.Open(texturePath));
    return xiiTextureAssetFile::WriteImage(file, image, {}, xiiAssetFileHeader());
  };
  if (!XII_TEST_RESULT(writeTexture(255)))
    return;
  const auto sampledTexture   = xiiResourceManager::LoadResource<xiiTexture2DResource>(texturePath);
  const auto previousFallback = xiiResourceManager::GetResourceTypeMissingFallback<xiiTexture2DResource>();
  if (builtin)
    xiiResourceManager::SetResourceTypeMissingFallback<xiiTexture2DResource>(sampledTexture);
  XII_SCOPE_EXIT(if (builtin) xiiResourceManager::SetResourceTypeMissingFallback<xiiTexture2DResource>(previousFallback));
  xiiMaterialResourceDescriptor description;
  description.m_BaseColor = xiiColor::Green; // The shader must override this native fallback.
  if (masked)
    description.m_AlphaMode = xiiMaterialAlphaMode::Mask;
  if (transparent)
  {
    // Shader authoring selects permutations. Native extraction must derive its
    // transparency classification instead of requiring duplicate native edits.
    auto& blend = description.m_PermutationVariables.ExpandAndGetRef();
    blend.m_sName.Assign("BLEND_MODE");
    blend.m_sValue.Assign("BLEND_MODE_TRANSPARENT");
    description.m_ShadingModel = xiiMaterialShadingModel::Unlit;
    description.m_BaseColor.a  = 0.5f;
  }
  description.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>(builtin ? "Shaders/Materials/DefaultMaterial.xiiShader" : graph ? "GraphicsFoundationTest/Shaders/NativeGraphSurface.xiiShader" :
                                                                                                                                                "GraphicsFoundationTest/Shaders/AuthoredSurface.xiiShader");
  if (native)
    description.m_hShader.Invalidate();
  auto& tint = description.m_Parameters.ExpandAndGetRef();
  tint.m_Name.Assign("AuthoredTint");
  tint.m_Value = xiiVec3(1, 0, 0);
  auto& gain   = description.m_Parameters.ExpandAndGetRef();
  gain.m_Name.Assign("AuthoredGain");
  gain.m_Value  = 1.0f;
  auto& opacity = description.m_Parameters.ExpandAndGetRef();
  opacity.m_Name.Assign("AuthoredOpacity");
  opacity.m_Value = 1.0f;
  auto& offset    = description.m_Parameters.ExpandAndGetRef();
  offset.m_Name.Assign("AuthoredOffset");
  offset.m_Value = xiiVec3::MakeZero();
  auto& texture  = description.m_Texture2DBindings.ExpandAndGetRef();
  texture.m_Name.Assign("AuthoredTexture");
  texture.m_Value        = sampledTexture;
  auto secondDescription = description;
  if (builtin)
  {
    auto& color = description.m_Parameters.ExpandAndGetRef();
    color.m_Name.Assign("BaseColor");
    color.m_Value = xiiColor(1, 0, 0);
  }
  const auto                         material = xiiResourceManager::CreateResource<xiiMaterialResource>(identity, std::move(description));
  xiiShaderPermutationResourceHandle reloadedPermutation;
  xiiMaterialShaderBinding           retainedBinding;
  if (!graph && !transparent && !native && !masked && !builtin)
  {
    xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::BlockTillLoaded);
    xiiMaterialInstanceSnapshot          snapshot;
    resource->GetDefaultInstance()->CreateSnapshot(snapshot);
    auto variables = snapshot.m_pSchema->GetPermutations();
    for (auto key : {"FORWARD_DIRECTIONAL_SHADOWS", "FORWARD_ATMOSPHERE", "FORWARD_REFRACTION", "FORWARD_SPOT_SHADOWS", "BLEND_MODE", "RENDER_PASS"})
    {
      xiiHashedString name, value;
      name.Assign(key);
      value.Assign(xiiStringView(key) == "BLEND_MODE" ? "BLEND_MODE_OPAQUE" : xiiStringView(key) == "RENDER_PASS" ? "RENDER_PASS_FORWARD" :
                                                                                                                    "FALSE");
      variables.Insert(name, value);
    }
    reloadedPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(snapshot.m_pSchema->GetShader(), variables, false);
    xiiResourceLock<xiiShaderPermutationResource> shader(reloadedPermutation, xiiResourceAcquireMode::BlockTillLoaded);
    XII_TEST_STATUS(retainedBinding.Build(snapshot, *shader.GetPointer()));
    XII_TEST_BOOL(retainedBinding.m_ConstantStages == (xiiGALShaderType::Vertex | xiiGALShaderType::Pixel));
    if (!XII_TEST_BOOL(retainedBinding.m_Constants.GetCount() >= 20))
      return;
    float packed[4];
    xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(packed), retainedBinding.m_Constants.GetData(), sizeof(packed));
    XII_TEST_FLOAT(packed[0], 1, 0); // HLSL float + float3 occupies one register.
    XII_TEST_FLOAT(packed[1], 1, 0);
    XII_TEST_FLOAT(packed[2], 0, 0);
    XII_TEST_FLOAT(packed[3], 0, 0);
    xiiMaterialSchemaDescription wrong;
    wrong.m_sName      = "Incompatible authored shader";
    wrong.m_Parameters = snapshot.m_pSchema->GetParameters();
    wrong.m_Textures   = snapshot.m_pSchema->GetTextures();
    for (auto& parameter : wrong.m_Parameters)
      if (parameter.m_sName.GetString() == "AuthoredGain")
      {
        parameter.m_Type         = xiiMaterialParameterType::Bool;
        parameter.m_DefaultValue = false;
      }
    xiiSharedPtr<xiiMaterialSchema> invalid = XII_DEFAULT_NEW(xiiMaterialSchema);
    XII_TEST_RESULT(invalid->Build(wrong));
    auto incompatible      = snapshot;
    incompatible.m_pSchema = invalid;
    xiiMaterialShaderBinding rejected;
    const auto               status = rejected.Build(incompatible, *shader.GetPointer());
    XII_TEST_BOOL(status.Failed());
    XII_TEST_BOOL(status.GetMessageString().FindSubString("AuthoredGain") != nullptr);
    XII_TEST_BOOL(!rejected.m_Pixel);
  }
  xiiPlatformProfile profile;
  profile.AddMissingConfigs();
  MaterialPreviewTestContext document;
  const auto                 initialized = document.Initialize(guid, {}, nullptr, "Material");
  XII_TEST_STATUS(initialized);
  if (initialized.Failed())
    return;
  XII_SCOPE_EXIT(document.Deinitialize());
  auto* context = document.CreateViewContext();
  if (!XII_TEST_BOOL(context != nullptr))
    return;
  XII_SCOPE_EXIT(document.DestroyViewContext(context));
  context->SetRenderPipelineProfile(&profile);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {128, 128};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto output        = device->CreateTexture(target);
  if (!XII_TEST_BOOL(output != nullptr))
    return;
  context->SetupRenderTarget(nullptr, output->GetDefaultView(xiiGALTextureViewType::RenderTarget), 128, 128);
  document.UpdateThumbnailViewContext(context);
  auto* world = document.GetWorld();
  XII_LOCK(world->GetWriteMarker());
  world->SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  if (!XII_TEST_BOOL(world->GetOrCreateModule<xiiRenderWorldModule>()->TryGetView(context->GetViewHandle(), view)))
    return;
  if (mixed)
  {
    for (auto& parameter : secondDescription.m_Parameters)
      if (parameter.m_Name.GetString() == "AuthoredTint")
        parameter.m_Value = xiiVec3(1, 1, 0);
    const auto                    secondMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>(xiiStringBuilder(identity, ".Second"), std::move(secondDescription));
    xiiMaterialResourceDescriptor standardDescription;
    standardDescription.m_BaseColor = xiiColor(0, 1, 0);
    const auto standard             = xiiResourceManager::CreateResource<xiiMaterialResource>(xiiStringBuilder(identity, ".Standard"), std::move(standardDescription));
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiGameObjectDescription object;
      object.m_bDynamic      = true;
      object.m_LocalPosition = view->GetCamera()->GetDirRight() * (i ? 0.075f : -0.075f) - view->GetCamera()->GetDirForwards() * 0.1f;
      object.m_LocalScaling  = xiiVec3(0.25f);
      xiiGameObject* owner   = nullptr;
      world->CreateObject(object, owner);
      xiiMeshComponent* mesh = nullptr;
      xiiMeshComponent::CreateComponent(owner, mesh);
      mesh->SetMesh(xiiResourceManager::GetExistingResource<xiiMeshResource>("SphereMaterialPreviewMesh"));
      mesh->SetMaterialOverride(0, i ? secondMaterial : standard);
    }
  }
  view->SetViewRenderMode(transparent ? xiiViewRenderMode::None : xiiViewRenderMode::BaseColor);
  view->SetPipelineParameter("Editor.SelectionOverlay", false);
  view->SetPipelineParameter("Editor.ShapeIcons", false);
  view->SetPipelineParameter("Editor.InfiniteGrid", false);
  if (transparent)
  {
    xiiDocumentConfigMsgToEngine scene;
    scene.m_sWhatToDo = "PreviewScene";
    scene.m_iValue    = 0;
    document.HandleMessage(&scene);
    auto settings        = xiiPreviewLightingSettings::MakePreset(xiiPreviewLightingSettings::Preset::Diagnostic);
    settings.m_fExposure = 1.0f;
    xiiSimpleDocumentConfigMsgToEngine lighting;
    lighting.m_sWhatToDo    = "PreviewLighting";
    lighting.m_PayloadValue = settings.ToVariant();
    document.HandleMessage(&lighting);
  }
  const auto render = [&](xiiUInt32& pixel) {
    for (xiiUInt32 frame = 0; frame < 4; ++frame)
    {
      context->Redraw(false);
      world->Update();
      device->WaitIdle();
    }
    if (!XII_TEST_RESULT(view->GetLastRenderResult()))
      return false;
    return xiiGpuTestReadback::ReadTexturePixel(*device, output.Borrow(), 64, 64, pixel);
  };
  xiiUInt32 red = 0, blue = 0;
  if (!XII_TEST_BOOL(render(red)))
    return;
  const xiiSceneMeshPickingPass* picking  = nullptr;
  const auto&                    pipeline = view->GetRenderPipelineInstance();
  if (const auto* resource = pipeline.GetPipeline())
    for (xiiUInt32 i = 0; i < resource->GetDescriptor().m_Passes.GetCount(); ++i)
      if (const auto* pass = xiiDynamicCast<const xiiSceneMeshPickingPass*>(pipeline.GetPass(i)))
        picking = pass;
  if (!XII_TEST_BOOL(picking != nullptr))
    return;
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_TEST_RESULT(picking->Pick(*view, 64, 64, hit));
  XII_TEST_BOOL(!hit.m_Source.m_hOwnerObject.IsInvalidated());
  if (mixed)
  {
    xiiDynamicArray<xiiColorGammaUB> pixels;
    if (!XII_TEST_BOOL(xiiGpuTestReadback::ReadTexturePixels(*device, output, pixels)))
      return;
    xiiUInt32 green = 0, yellow = 0, redCount = 0;
    for (const auto pixel : pixels)
    {
      green += pixel.r < 10 && pixel.g > 200 && pixel.b < 10;
      yellow += pixel.r > 200 && pixel.g > 200 && pixel.b < 10;
      redCount += pixel.r > 200 && pixel.g < 10 && pixel.b < 10;
    }
    XII_TEST_BOOL_MSG(green > 8 && yellow > 8 && redCount > 50, "Mixed native and two authored bins: green=%u yellow=%u red=%u", green, yellow, redCount);
    return;
  }
  if (native)
  {
    XII_TEST_BOOL_MSG((red & 255) < 10 && ((red >> 8) & 255) > 100 && ((red >> 16) & 255) < 10, "Native transparent control: %08x", red);
    return;
  }
  if (graph)
  {
    if (transparent)
    {
      XII_TEST_BOOL_MSG((red & 255) > 100 && ((red >> 8) & 255) > 100 && ((red >> 16) & 255) > 100, "Generated graph must execute transparent native shading: %08x", red);
      return;
    }
    XII_TEST_BOOL_MSG((red & 255) > 240 && ((red >> 8) & 255) > 240 && ((red >> 16) & 255) > 240, "Generated graph must replace native green with white: %08x", red);
    view->SetViewRenderMode(xiiViewRenderMode::Roughness);
    xiiUInt32 roughness = 0;
    if (!XII_TEST_BOOL(render(roughness)))
      return;
    XII_TEST_BOOL_MSG((roughness & 255) > 80 && (roughness & 255) < 200, "Generated 0.42 roughness must reach the G-buffer: %08x", roughness);
    return;
  }
  XII_TEST_BOOL_MSG((red & 255) > 200 && ((red >> 8) & 255) < 10 && ((red >> 16) & 255) < 10, "Authored red must replace native green: %08x", red);
  {
    xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::BlockTillLoaded);
    resource->SetParameter("AuthoredTint", xiiVec3(0, 0, 1));
    resource->SetParameter("AuthoredGain", 0.5f);
    if (builtin)
      resource->SetParameter("BaseColor", xiiColor(0, 0, 0.5f));
  }
  if (!XII_TEST_BOOL(render(blue)))
    return;
  XII_TEST_BOOL_MSG((blue & 255) < 10 && ((blue >> 8) & 255) < 10 && ((blue >> 16) & 255) > 100, "Runtime authored constants must update: %08x", blue);
  if (builtin)
    return;
  if (!XII_TEST_RESULT(writeTexture(0)))
    return;
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(sampledTexture, true));
  xiiUInt32 black = 0;
  if (!XII_TEST_BOOL(render(black)))
    return;
  XII_TEST_INT(black & 0xFFFFFFu, 0);
  if (!XII_TEST_RESULT(writeTexture(255)))
    return;
  XII_TEST_BOOL(xiiResourceManager::ReloadResource(sampledTexture, true));
  xiiUInt32 restored = 0;
  if (!XII_TEST_BOOL(render(restored)))
    return;
  XII_TEST_INT(restored, blue);
  if (reloadedPermutation.IsValid())
  {
    XII_TEST_BOOL(xiiResourceManager::ReloadResource(reloadedPermutation, true));
    xiiUInt32 reloaded = 0;
    if (!XII_TEST_BOOL(render(reloaded)))
      return;
    XII_TEST_INT(reloaded, blue);
    XII_TEST_BOOL(retainedBinding.m_Pixel != nullptr);
    XII_TEST_BOOL(!retainedBinding.AreTexturesCurrent());
    {
      xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::BlockTillLoaded);
      resource->SetParameter("AuthoredOffset", xiiVec3(1, 0, 0));
    }
    xiiUInt32 displaced = 0;
    if (!XII_TEST_BOOL(render(displaced)))
      return;
    XII_TEST_INT(displaced & 0xFFFFFFu, 0);
    XII_TEST_RESULT(picking->Pick(*view, 64, 64, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
    view->SetViewRenderMode(xiiViewRenderMode::Wireframe);
    if (!XII_TEST_BOOL(render(displaced)))
      return;
    XII_TEST_INT(displaced & 0xFFFFFFu, 0);
    {
      xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::BlockTillLoaded);
      resource->SetParameter("AuthoredOffset", xiiVec3::MakeZero());
    }
    if (!XII_TEST_BOOL(render(restored)))
      return;
    xiiDynamicArray<xiiColorGammaUB> wireframe;
    if (!XII_TEST_BOOL(xiiGpuTestReadback::ReadTexturePixels(*device, output, wireframe)))
      return;
    xiiUInt32 lines = 0;
    for (const auto pixel : wireframe)
      lines += pixel.r > 20 || pixel.g > 20 || pixel.b > 20;
    XII_TEST_BOOL(lines > 20);
  }
  if (masked)
  {
    {
      xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::BlockTillLoaded);
      resource->SetParameter("AuthoredOpacity", 0.0f);
    }
    xiiUInt32 invisible = 0;
    if (!XII_TEST_BOOL(render(invisible)))
      return;
    XII_TEST_INT(invisible & 0xFFFFFFu, 0);
    XII_TEST_RESULT(picking->Pick(*view, 64, 64, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(MaterialShaders);
XII_CREATE_SIMPLE_TEST(MaterialShaders, HandwrittenDispatch) { TestAuthoredPreview(AuthoredPreview::Handwritten); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, GraphDispatch) { TestAuthoredPreview(AuthoredPreview::Graph); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, DefaultShaderDispatch) { TestAuthoredPreview(AuthoredPreview::DefaultShader); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, TransparentGraphDispatch) { TestAuthoredPreview(AuthoredPreview::GraphTransparent); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, TransparentDispatch) { TestAuthoredPreview(AuthoredPreview::Transparent); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, NativeTransparentControl) { TestAuthoredPreview(AuthoredPreview::NativeTransparent); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, MaskedDispatchAndPicking) { TestAuthoredPreview(AuthoredPreview::Masked); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, MixedMaterialBins) { TestAuthoredPreview(AuthoredPreview::Mixed); }
XII_CREATE_SIMPLE_TEST(MaterialShaders, FixedAndInheritedClassification)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiMaterialResourceDescriptor fixed;
  fixed.m_hShader   = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Materials/FallbackMaterial.xiiShader");
  fixed.m_BlendMode = xiiMaterialBlendMode::Translucent;
  auto& selection   = fixed.m_PermutationVariables.ExpandAndGetRef();
  selection.m_sName.Assign("BLEND_MODE");
  selection.m_sValue.Assign("BLEND_MODE_TRANSPARENT");
  const auto fixedMaterial = xiiResourceManager::CreateResource<xiiMaterialResource>("MaterialPreview.FixedClassification", std::move(fixed));
  {
    xiiResourceLock<xiiMaterialResource> resource(fixedMaterial, xiiResourceAcquireMode::BlockTillLoaded);
    XII_TEST_BOOL(!resource->GetRuntimeState().IsTranslucent());
    XII_TEST_BOOL(!resource->GetDefaultInstance()->GetRuntimeState().IsTranslucent());
    const auto* blend = resource->GetSchema()->GetPermutations().GetValue(xiiTempHashedString("BLEND_MODE"));
    XII_TEST_BOOL(blend && blend->GetString() == "BLEND_MODE_OPAQUE");
  }
  xiiMaterialResourceDescriptor parent;
  parent.m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("GraphicsFoundationTest/Shaders/AuthoredSurface.xiiShader");
  auto& blend      = parent.m_PermutationVariables.ExpandAndGetRef();
  blend.m_sName.Assign("BLEND_MODE");
  blend.m_sValue.Assign("BLEND_MODE_TRANSPARENT");
  auto& twoSided = parent.m_PermutationVariables.ExpandAndGetRef();
  twoSided.m_sName.Assign("TWO_SIDED");
  twoSided.m_sValue.Assign("TRUE");
  auto& shading = parent.m_PermutationVariables.ExpandAndGetRef();
  shading.m_sName.Assign("SHADING_MODE");
  shading.m_sValue.Assign("SHADING_MODE_FULLBRIGHT");
  const auto                    base = xiiResourceManager::CreateResource<xiiMaterialResource>("MaterialPreview.ParentClassification", std::move(parent));
  xiiMaterialResourceDescriptor child;
  child.m_hBaseMaterial                          = base;
  const auto                           inherited = xiiResourceManager::CreateResource<xiiMaterialResource>("MaterialPreview.InheritedClassification", std::move(child));
  xiiResourceLock<xiiMaterialResource> resource(inherited, xiiResourceAcquireMode::BlockTillLoaded);
  const auto                           state = resource->GetRuntimeState();
  XII_TEST_BOOL(state.IsTranslucent() && state.IsTwoSided() && state.m_ShadingModel == xiiMaterialShadingModel::Unlit);
  XII_TEST_BOOL(!state.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::ReceivesLighting));
  const auto instance = resource->GetDefaultInstance()->GetRuntimeState();
  XII_TEST_BOOL(instance.IsTranslucent() && instance.IsTwoSided() && instance.m_ShadingModel == xiiMaterialShadingModel::Unlit);
}
