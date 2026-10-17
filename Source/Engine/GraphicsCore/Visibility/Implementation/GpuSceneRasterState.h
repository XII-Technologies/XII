/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>

class xiiGpuSceneRasterManagerState
{
public:
  xiiShaderPermutationResourceHandle m_hGBufferShaderPermutation;
  xiiShaderPermutationResourceHandle m_hForwardPlusShaderPermutation;
  xiiSharedPtr<xiiGALSampler>        m_pMaterialSampler;
  bool                               m_bEngineStarted        = false;
  bool                               m_bMeshShadersSupported = false;
};
