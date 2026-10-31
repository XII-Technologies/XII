/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/Status.h>
#include <GraphicsCore/Material/MaterialInstance.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>

/// Immutable reflected binding snapshot. Holds GPU objects through graph execution,
/// including when the source shader or texture reloads after frame construction.
struct XII_GRAPHICSCORE_DLL xiiMaterialShaderBinding
{
  struct Texture
  {
    xiiHashedString              m_Name;
    xiiSharedPtr<xiiGALTexture>  m_Texture;
    xiiSharedPtr<xiiGALSampler>  m_Sampler;
    xiiTexture2DResourceHandle   m_hTexture2D;
    xiiTextureCubeResourceHandle m_hTextureCube;
  };
  xiiSharedPtr<xiiGALShader>                    m_Vertex, m_Pixel;
  xiiSharedPtr<xiiGALPipelineResourceSignature> m_Signature;
  xiiSharedPtr<xiiGALBlendState>                m_Blend;
  xiiSharedPtr<xiiGALDepthStencilState>         m_Depth;
  xiiSharedPtr<xiiGALRasterizerState>           m_Rasterizer;
  xiiDynamicArray<xiiUInt8>                     m_Constants;
  xiiDynamicArray<Texture>                      m_Textures;
  xiiBitflags<xiiGALShaderType>                 m_ConstantStages;

  /// Validates the canonical native G-buffer interface and packs constants by
  /// reflected names/offsets rather than assuming schema and HLSL packing agree.
  xiiStatus Build(const xiiMaterialInstanceSnapshot& snapshot, const xiiShaderPermutationResource& shader, bool bTransparent = false, bool bPicking = false);
  void      Bind(xiiGALCommandList& commands, xiiGALBuffer* constants) const;
  bool      AreTexturesCurrent() const;
};
