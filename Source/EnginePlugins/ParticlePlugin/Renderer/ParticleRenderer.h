#pragma once

#include <Foundation/Math/Color16f.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

class xiiRenderContext;

/// \brief Implements rendering of particle systems
class XII_PARTICLEPLUGIN_DLL xiiParticleRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleRenderer);

public:
  xiiParticleRenderer();
  ~xiiParticleRenderer();

  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;

protected:
  struct TempSystemCB
  {
    TempSystemCB(xiiRenderContext* pRenderContext);
    ~TempSystemCB();

    void SetGenericData(bool bApplyObjectTransform, const xiiTransform& objectTransform, xiiTime effectLifeTime, xiiUInt8 uiNumVariationsX, xiiUInt8 uiNumVariationsY, xiiUInt8 uiNumFlipbookAnimsX, xiiUInt8 uiNumFlipbookAnimsY, float fDistortionStrength = 0);
    void SetTrailData(float fSnapshotFraction, xiiInt32 iNumUsedTrailPoints);

    xiiConstantBufferStorage<xiiParticleSystemConstants>* m_pConstants;
    xiiConstantBufferStorageHandle                        m_hConstantBuffer;
  };

  void CreateParticleDataBuffer(xiiGALBufferHandle& inout_hBuffer, xiiUInt32 uiDataTypeSize, xiiUInt32 uiNumParticlesPerBatch);
  void DestroyParticleDataBuffer(xiiGALBufferHandle& inout_hBuffer);
  void BindParticleShader(xiiRenderContext* pRenderContext, const char* szShader) const;

protected:
  xiiShaderResourceHandle m_hShader;
};
