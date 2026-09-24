/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Material/MaterialSystem.h>

/// This compiled example demonstrates generated material authoring, frequent updates and graph use.
/// Applications would retain the returned schema, instance and handle in their scene/simulation data.
namespace xiiMaterialExamples
{
  struct ExampleMaterial
  {
    xiiSharedPtr<xiiMaterialSchema>   m_pSchema;
    xiiSharedPtr<xiiMaterialInstance> m_pInstance;
    xiiMaterialGpuHandle              m_GpuHandle;
  };

  [[maybe_unused]] static xiiResult CreateXRayVolumeMaterial(xiiMaterialSystem& materialSystem, const xiiShaderResourceHandle& hShader, const xiiTexture2DResourceHandle& hDensityVolumeSlice, ExampleMaterial& out_material)
  {
    xiiMaterialSchemaDescription schema;
    schema.m_sName        = "XRay Volume Material";
    schema.m_hShader      = hShader;
    schema.m_Domain       = xiiMaterialDomain::Volume;
    schema.m_ShadingModel = xiiMaterialShadingModel::XRayAttenuation;

    auto& attenuation = schema.AddParameter("Attenuation", xiiMaterialParameterType::Float, 0.85f);
    attenuation.m_sCategory = "Medical Imaging";
    attenuation.m_MinValue  = 0.0f;
    attenuation.m_MaxValue  = 10.0f;
    attenuation.m_UpdateFrequency = xiiMaterialUpdateFrequency::PerFrame;
    attenuation.m_Flags.Add(xiiMaterialParameterFlags::Animatable);

    schema.AddParameter("Tint", xiiMaterialParameterType::Color, xiiColor(0.65f, 0.85f, 1.0f));
    auto& densityTexture = schema.AddTexture("DensityTexture", xiiGALShaderTextureType::Texture2D);
    densityTexture.m_bRequired = true;

    xiiMaterialRuntimeState state;
    state.m_FeatureFlags.Add(xiiMaterialFeatureFlags::UsesBindlessResources);
    XII_SUCCEED_OR_RETURN(xiiMaterialSystem::CreateRuntimeMaterial(schema, state, out_material.m_pSchema, out_material.m_pInstance));
    XII_SUCCEED_OR_RETURN(out_material.m_pInstance->SetTexture2D(densityTexture.m_Id, hDensityVolumeSlice));

    // Simulation may update this every frame; only the affected instance revision is uploaded.
    XII_SUCCEED_OR_RETURN(out_material.m_pInstance->SetParameter(attenuation.m_Id, 1.2f));
    out_material.m_GpuHandle = materialSystem.RegisterMaterial(out_material.m_pInstance);
    return out_material.m_GpuHandle.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }

  [[maybe_unused]] static xiiRenderGraphBufferHandle PrepareMaterialFrame(xiiMaterialSystem& materialSystem, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
  {
    materialSystem.BeginFrame(uiFrameIndex, uiCompletedFrame);

    // A later graphics/compute pass calls builder.ReadBuffer() on this returned version. The graph
    // then inserts the copy-to-shader transition and any transfer-queue ownership synchronization.
    return materialSystem.AddUploadPass(graph);
  }
} // namespace xiiMaterialExamples

