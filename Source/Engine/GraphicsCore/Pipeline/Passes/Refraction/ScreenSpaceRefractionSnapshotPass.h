/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceRefractionSnapshotPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceRefractionSnapshotPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputHDRSceneColor{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::None, xiiGALResourceDimension::Texture2D, 1};
  xiiRenderPipelineTextureOutputPin m_OutputRefractionSceneColorInput{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};

  xiiScreenSpaceRefractionSnapshotPass();
  ~xiiScreenSpaceRefractionSnapshotPass();

  xiiResult Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  void Setup(Data& data, xiiRenderGraphBuilder& builder);
  void Execute(const Data& data, xiiRenderGraphPassContext& context);
};
