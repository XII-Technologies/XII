#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <GraphicsFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/Pass.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

struct xiiRenderWorldRenderEvent;

//////////////////////////////////////////////////////////////////////////
// xiiRenderContext
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiRenderContext
{
private:
  xiiRenderContext();
  ~xiiRenderContext();
  friend class xiiMemoryUtils;

  static xiiRenderContext*                    s_pDefaultInstance;
  static xiiHybridArray<xiiRenderContext*, 4> s_Instances;

public:
  static xiiRenderContext* GetDefaultInstance();
  static xiiRenderContext* CreateInstance();
  static void              DestroyInstance(xiiRenderContext* pRenderer);

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    xiiUInt32 m_uiFailedDrawcalls;
  };

  Statistics GetAndResetStatistics();

  xiiGALRenderCommandEncoder* BeginRendering(xiiGALPass* pGALPass, const xiiGALRenderingSetup& renderingSetup, const xiiRectFloat& viewport, const char* szName = "", bool bStereoRendering = false);
  void                        EndRendering();

  xiiGALComputeCommandEncoder* BeginCompute(xiiGALPass* pGALPass, const char* szName = "");
  void                         EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <typename T>
  class CommandEncoderScope
  {
    XII_DISALLOW_COPY_AND_ASSIGN(CommandEncoderScope);

  public:
    XII_ALWAYS_INLINE ~CommandEncoderScope()
    {
      m_RenderContext.EndCommandEncoder(m_pGALCommandEncoder);

      if (m_pGALPass != nullptr)
      {
        xiiGALDevice::GetDefaultDevice()->EndPass(m_pGALPass);
      }
    }

    XII_ALWAYS_INLINE T* operator->() { return m_pGALCommandEncoder; }
    XII_ALWAYS_INLINE    operator const T*() { return m_pGALCommandEncoder; }

  private:
    friend class xiiRenderContext;

    XII_ALWAYS_INLINE CommandEncoderScope(xiiRenderContext& renderContext, xiiGALPass* pGALPass, T* pGALCommandEncoder) :
      m_RenderContext(renderContext), m_pGALPass(pGALPass), m_pGALCommandEncoder(pGALCommandEncoder)
    {
    }

    xiiRenderContext& m_RenderContext;
    xiiGALPass*       m_pGALPass;
    T*                m_pGALCommandEncoder;
  };

  using RenderingScope = CommandEncoderScope<xiiGALRenderCommandEncoder>;
  XII_ALWAYS_INLINE static RenderingScope BeginRenderingScope(xiiGALPass* pGALPass, const xiiRenderViewContext& viewContext, const xiiGALRenderingSetup& renderingSetup, const char* szName = "", bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, szName, bStereoRendering));
  }

  XII_ALWAYS_INLINE static RenderingScope BeginPassAndRenderingScope(const xiiRenderViewContext& viewContext, const xiiGALRenderingSetup& renderingSetup, const char* szName, bool bStereoRendering = false)
  {
    xiiGALPass* pGALPass = xiiGALDevice::GetDefaultDevice()->BeginPass(szName);

    return RenderingScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering));
  }

  using ComputeScope = CommandEncoderScope<xiiGALComputeCommandEncoder>;
  XII_ALWAYS_INLINE static ComputeScope BeginComputeScope(xiiGALPass* pGALPass, const xiiRenderViewContext& viewContext, const char* szName = "")
  {
    return ComputeScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginCompute(pGALPass, szName));
  }

  XII_ALWAYS_INLINE static ComputeScope BeginPassAndComputeScope(const xiiRenderViewContext& viewContext, const char* szName)
  {
    xiiGALPass* pGALPass = xiiGALDevice::GetDefaultDevice()->BeginPass(szName);

    return ComputeScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginCompute(pGALPass));
  }

  XII_ALWAYS_INLINE xiiGALCommandEncoder* GetCommandEncoder()
  {
    XII_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr, "BeginRendering/Compute has not been called");
    return m_pGALCommandEncoder;
  }

  XII_ALWAYS_INLINE xiiGALRenderCommandEncoder* GetRenderCommandEncoder()
  {
    XII_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && !m_bCompute, "BeginRendering has not been called");
    return static_cast<xiiGALRenderCommandEncoder*>(m_pGALCommandEncoder);
  }

  XII_ALWAYS_INLINE xiiGALComputeCommandEncoder* GetComputeCommandEncoder()
  {
    XII_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && m_bCompute, "BeginCompute has not been called");
    return static_cast<xiiGALComputeCommandEncoder*>(m_pGALCommandEncoder);
  }


  // Member Functions
  void SetShaderPermutationVariable(const char* szName, const xiiTempHashedString& sValue);
  void SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue);

  void BindMaterial(const xiiMaterialResourceHandle& hMaterial);

  void BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  void BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  void BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);

  void BindTexture2D(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView);
  void BindTexture3D(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView);
  void BindTextureCube(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView);

  /// Binds a read+write texture or buffer
  void BindUAV(const xiiTempHashedString& sSlotName, xiiGALUnorderedAccessViewHandle hUnorderedAccessViewHandle);

  void BindSamplerState(const xiiTempHashedString& sSlotName, xiiGALSamplerStateHandle hSamplerSate);

  void BindBuffer(const xiiTempHashedString& sSlotName, xiiGALResourceViewHandle hResourceView);

  void BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiGALBufferHandle hConstantBuffer);
  void BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiConstantBufferStorageHandle hConstantBufferStorage);

  /// \brief Sets the currently active shader on the given render context.
  ///
  /// This function has no effect until the next draw or dispatch call on the context.
  void BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags = xiiShaderBindFlags::Default);

  void                   BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void                   BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer);
  void                   BindMeshBuffer(xiiGALBufferHandle hVertexBuffer, xiiGALBufferHandle hIndexBuffer, const xiiVertexDeclarationInfo* pVertexDeclarationInfo, xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiPrimitiveCount, xiiGALBufferHandle hVertexBuffer2 = {}, xiiGALBufferHandle hVertexBuffer3 = {}, xiiGALBufferHandle hVertexBuffer4 = {});
  XII_ALWAYS_INLINE void BindNullMeshBuffer(xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiPrimitiveCount)
  {
    BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, topology, uiPrimitiveCount);
  }

  xiiResult DrawMeshBuffer(xiiUInt32 uiPrimitiveCount = 0xFFFFFFFF, xiiUInt32 uiFirstPrimitive = 0, xiiUInt32 uiInstanceCount = 1);

  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1, xiiUInt32 uiThreadGroupCountZ = 1);

  xiiResult ApplyContextStates(bool bForce = false);
  void      ResetContextState();

  xiiGlobalConstants&       WriteGlobalConstants();
  const xiiGlobalConstants& ReadGlobalConstants() const;

  /// \brief Sets the texture filter mode that is used by default for texture resources.
  ///
  /// The built in default is Anisotropic 4x.
  /// If the default setting is changed, already loaded textures might not adjust.
  /// Nearest filtering is not allowed as a default filter.
  void SetDefaultTextureFilter(xiiTextureFilterSetting::Enum filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  xiiTextureFilterSetting::Enum GetDefaultTextureFilter() const { return m_DefaultTextureFilter; }

  /// \brief Returns the 'fixed' texture filter setting that the combination of default texture filter and given \a configuration defines.
  ///
  /// If \a configuration is set to a fixed filter, that setting is returned.
  /// If it is one of LowestQuality to HighestQuality, the adjusted default filter is returned.
  /// When the default filter is used (with adjustments), the allowed range is Bilinear to Aniso16x, the Nearest filter is never used.
  xiiTextureFilterSetting::Enum GetSpecificTextureFilter(xiiTextureFilterSetting::Enum configuration) const;

  /// \brief Set async shader loading. During runtime all shaders should be preloaded so this is off by default.
  void SetAllowAsyncShaderLoading(bool bAllow);

  /// \brief Returns async shader loading. During runtime all shaders should be preloaded so this is off by default.
  bool GetAllowAsyncShaderLoading();


  // Static Functions
public:
  // Constant buffer storage handling
  template <typename T>
  XII_ALWAYS_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage()
  {
    return CreateConstantBufferStorage(sizeof(T));
  }

  template <typename T>
  XII_FORCE_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiConstantBufferStorage<T>*& out_pStorage)
  {
    xiiConstantBufferStorageBase*  pStorage;
    xiiConstantBufferStorageHandle hStorage = CreateConstantBufferStorage(sizeof(T), pStorage);
    out_pStorage                            = static_cast<xiiConstantBufferStorage<T>*>(pStorage);
    return hStorage;
  }

  XII_FORCE_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes)
  {
    xiiConstantBufferStorageBase* pStorage;
    return CreateConstantBufferStorage(uiSizeInBytes, pStorage);
  }

  static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes, xiiConstantBufferStorageBase*& out_pStorage);
  static void                           DeleteConstantBufferStorage(xiiConstantBufferStorageHandle hStorage);

  template <typename T>
  XII_FORCE_INLINE static bool TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorage<T>*& out_pStorage)
  {
    xiiConstantBufferStorageBase* pStorage = nullptr;
    bool                          bResult  = TryGetConstantBufferStorage(hStorage, pStorage);
    out_pStorage                           = static_cast<xiiConstantBufferStorage<T>*>(pStorage);
    return bResult;
  }

  static bool TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorageBase*& out_pStorage);

  template <typename T>
  XII_FORCE_INLINE static T* GetConstantBufferData(xiiConstantBufferStorageHandle hStorage)
  {
    xiiConstantBufferStorage<T>* pStorage = nullptr;
    if (TryGetConstantBufferStorage(hStorage, pStorage))
    {
      return &(pStorage->GetDataForWriting());
    }

    return nullptr;
  }

  // Default sampler state
  static xiiGALSamplerStateHandle GetDefaultSamplerState(xiiBitflags<xiiDefaultSamplerFlags> flags);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RendererContext);

  static void LoadBuiltinShader(xiiShaderUtils::xiiBuiltinShaderType type, xiiShaderUtils::xiiBuiltinShader& out_shader);
  static void OnEngineShutdown();

private:
  Statistics                         m_Statistics;
  xiiBitflags<xiiRenderContextFlags> m_StateFlags;
  xiiShaderResourceHandle            m_hActiveShader;
  xiiGALShaderHandle                 m_hActiveGALShader;

  xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
  xiiMaterialResourceHandle                      m_hNewMaterial;
  xiiMaterialResourceHandle                      m_hMaterial;

  xiiShaderPermutationResourceHandle m_hActiveShaderPermutation;

  xiiBitflags<xiiShaderBindFlags> m_ShaderBindFlags;

  xiiGALBufferHandle               m_hVertexBuffers[4];
  xiiGALBufferHandle               m_hIndexBuffer;
  const xiiVertexDeclarationInfo*  m_pVertexDeclarationInfo;
  xiiGALPrimitiveTopology::Enum    m_Topology;
  xiiUInt32                        m_uiMeshBufferPrimitiveCount;
  xiiEnum<xiiTextureFilterSetting> m_DefaultTextureFilter;
  bool                             m_bAllowAsyncShaderLoading;
  bool                             m_bStereoRendering = false;

  xiiHashTable<xiiUInt64, xiiGALResourceViewHandle>        m_BoundTextures2D;
  xiiHashTable<xiiUInt64, xiiGALResourceViewHandle>        m_BoundTextures3D;
  xiiHashTable<xiiUInt64, xiiGALResourceViewHandle>        m_BoundTexturesCube;
  xiiHashTable<xiiUInt64, xiiGALUnorderedAccessViewHandle> m_BoundUAVs;
  xiiHashTable<xiiUInt64, xiiGALSamplerStateHandle>        m_BoundSamplers;
  xiiHashTable<xiiUInt64, xiiGALResourceViewHandle>        m_BoundBuffer;

  struct BoundConstantBuffer
  {
    XII_DECLARE_POD_TYPE();

    BoundConstantBuffer() = default;
    BoundConstantBuffer(xiiGALBufferHandle hConstantBuffer) :
      m_hConstantBuffer(hConstantBuffer)
    {
    }
    BoundConstantBuffer(xiiConstantBufferStorageHandle hConstantBufferStorage) :
      m_hConstantBufferStorage(hConstantBufferStorage)
    {
    }

    xiiGALBufferHandle             m_hConstantBuffer;
    xiiConstantBufferStorageHandle m_hConstantBufferStorage;
  };

  xiiHashTable<xiiUInt64, BoundConstantBuffer> m_BoundConstantBuffers;

  xiiConstantBufferStorageHandle m_hGlobalConstantBufferStorage;

  struct ShaderVertexDecl
  {
    xiiGALShaderHandle m_hShader;
    xiiUInt32          m_uiVertexDeclarationHash;

    XII_FORCE_INLINE bool operator<(const ShaderVertexDecl& rhs) const
    {
      if (m_hShader < rhs.m_hShader)
        return true;
      if (rhs.m_hShader < m_hShader)
        return false;
      return m_uiVertexDeclarationHash < rhs.m_uiVertexDeclarationHash;
    }

    XII_FORCE_INLINE bool operator==(const ShaderVertexDecl& rhs) const
    {
      return (m_hShader == rhs.m_hShader && m_uiVertexDeclarationHash == rhs.m_uiVertexDeclarationHash);
    }
  };

  static xiiResult BuildVertexDeclaration(xiiGALShaderHandle hShader, const xiiVertexDeclarationInfo& decl, xiiGALVertexDeclarationHandle& out_Declaration);

  static xiiMap<ShaderVertexDecl, xiiGALVertexDeclarationHandle> s_GALVertexDeclarations;

  static xiiMutex                                                              s_ConstantBufferStorageMutex;
  static xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     s_FreeConstantBufferStorage;

  static xiiGALSamplerStateHandle s_hDefaultSamplerStates[4];

private: // Per Renderer States
  friend RenderingScope;
  friend ComputeScope;
  XII_ALWAYS_INLINE void EndCommandEncoder(xiiGALRenderCommandEncoder*) { EndRendering(); }
  XII_ALWAYS_INLINE void EndCommandEncoder(xiiGALComputeCommandEncoder*) { EndCompute(); }

  xiiGALPass*           m_pGALPass           = nullptr;
  xiiGALCommandEncoder* m_pGALCommandEncoder = nullptr;
  bool                  m_bCompute           = false;

  // Member Functions
  void UploadConstants();

  void                          SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue);
  void                          BindShaderInternal(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags);
  xiiShaderPermutationResource* ApplyShaderState();
  xiiMaterialResource*          ApplyMaterialState();
  void                          ApplyConstantBufferBindings(const xiiShaderStageBinary* pBinary);
  void                          ApplyTextureBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary);
  void                          ApplyUAVBindings(const xiiShaderStageBinary* pBinary);
  void                          ApplySamplerBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary);
  void                          ApplyBufferBindings(xiiGALShaderStage::Enum stage, const xiiShaderStageBinary* pBinary);
};
