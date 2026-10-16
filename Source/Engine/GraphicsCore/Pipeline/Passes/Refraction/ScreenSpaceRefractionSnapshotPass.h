/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

class XII_GRAPHICSCORE_DLL xiiScreenSpaceRefractionSnapshotPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenSpaceRefractionSnapshotPass, xiiRenderPipelinePass);

public:
  xiiScreenSpaceRefractionSnapshotPass();
  ~xiiScreenSpaceRefractionSnapshotPass();

  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;

private:
  struct Data;
  void Setup(Data& data, xiiRenderGraphBuilder& builder);
  void Execute(const Data& data, xiiRenderGraphPassContext& context);
};
