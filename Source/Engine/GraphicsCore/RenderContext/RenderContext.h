#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>
#include <GraphicsCore/RenderContext/RenderTargetSetup.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

#include <GraphicsCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

struct xiiRenderWorldRenderEvent;

//////////////////////////////////////////////////////////////////////////
// xiiRenderContext
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiRenderContext
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

  xiiGALCommandList* BeginRendering(const xiiGALRenderingSetup& renderingSetup, const xiiRectFloat& viewport, xiiStringView sName = {}, bool bStereoRendering = false);
  void               EndRendering();

  xiiGALCommandList* BeginCompute(xiiStringView sName = {});
  void               EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <typename T>
  class CommandListScope
  {
    XII_DISALLOW_COPY_AND_ASSIGN(CommandListScope);

  public:
    XII_ALWAYS_INLINE ~CommandListScope()
    {
      if (m_pCommandQueue != nullptr)
      {
        if (m_RenderContext.m_bCompute)
        {
          m_RenderContext.EndCompute();
        }
        else
        {
          m_RenderContext.EndRendering();
        }
      }
    }

    XII_ALWAYS_INLINE T* operator->() { return m_pCommandList; }
    XII_ALWAYS_INLINE    operator const T*() { return m_pCommandList; }

  private:
    friend class xiiRenderContext;

    XII_ALWAYS_INLINE CommandListScope(xiiRenderContext& renderContext, xiiGALCommandQueue* pCommandQueue, T* pGALCommandList) :
      m_RenderContext(renderContext), m_pCommandQueue(pCommandQueue), m_pCommandList(pGALCommandList)
    {
    }

    xiiRenderContext&   m_RenderContext;
    xiiGALCommandQueue* m_pCommandQueue = nullptr;
    T*                  m_pCommandList  = nullptr;
  };

  using RenderingScope = CommandListScope<xiiGALCommandList>;
  XII_ALWAYS_INLINE static RenderingScope BeginRenderingScope(const xiiRenderViewContext& viewContext, const xiiGALRenderingSetup& renderingSetup, xiiStringView sName = {}, bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginRendering(renderingSetup, viewContext.m_pViewData->m_ViewPortRect, sName, bStereoRendering));
  }

  XII_ALWAYS_INLINE static RenderingScope BeginPassAndRenderingScope(const xiiRenderViewContext& viewContext, const xiiGALRenderingSetup& renderingSetup, xiiStringView sName, bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, xiiRenderContext::GetDefaultInstance()->m_pCommandQueue, viewContext.m_pRenderContext->BeginRendering(renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering));
  }

  using ComputeScope = CommandListScope<xiiGALCommandList>;
  XII_ALWAYS_INLINE static ComputeScope BeginComputeScope(const xiiRenderViewContext& viewContext, xiiStringView sName = {})
  {
    return ComputeScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginCompute(sName));
  }

  XII_ALWAYS_INLINE static ComputeScope BeginPassAndComputeScope(const xiiRenderViewContext& viewContext, xiiStringView sName)
  {
    return ComputeScope(*viewContext.m_pRenderContext, xiiRenderContext::GetDefaultInstance()->m_pCommandQueue, viewContext.m_pRenderContext->BeginCompute(sName));
  }

  XII_ALWAYS_INLINE xiiGALCommandList* GetCommandList()
  {
    XII_ASSERT_DEBUG(m_pCommandList != nullptr, "BeginRendering/Compute has not been called");
    return m_pCommandList;
  }

  XII_ALWAYS_INLINE xiiGALCommandList* GetGraphicsCommandList()
  {
    XII_ASSERT_DEBUG(m_pCommandList != nullptr && !m_bCompute, "BeginRendering has not been called");
    return m_pCommandList;
  }

  XII_ALWAYS_INLINE xiiGALCommandList* GetComputeCommandList()
  {
    XII_ASSERT_DEBUG(m_pCommandList != nullptr && m_bCompute, "BeginCompute has not been called");
    return m_pCommandList;
  }


  // Member Functions
  void SetShaderPermutationVariable(xiiStringView sName, const xiiTempHashedString& sValue);
  void SetShaderPermutationVariable(const xiiHashedString& sName, const xiiHashedString& sValue);

  void BindMaterial(const xiiMaterialResourceHandle& hMaterial);

  void BindTexture2D(const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  void BindTexture3D(const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  void BindTextureCube(const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);

  void BindTexture2D(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView);
  void BindTexture3D(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView);
  void BindTextureCube(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hResourceView);

  /// Binds a read+write texture or buffer
  void BindBufferUAV(const xiiTempHashedString& sSlotName, xiiGALBufferViewHandle hUnorderedAccessViewHandle);
  void BindTextureUAV(const xiiTempHashedString& sSlotName, xiiGALTextureViewHandle hUnorderedAccessViewHandle);

  void BindSampler(const xiiTempHashedString& sSlotName, xiiGALSamplerHandle hSamplerSate);

  void BindBuffer(const xiiTempHashedString& sSlotName, xiiGALBufferViewHandle hResourceView);

  void BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiGALBufferHandle hConstantBuffer);
  void BindConstantBuffer(const xiiTempHashedString& sSlotName, xiiConstantBufferStorageHandle hConstantBufferStorage);

  /// \brief Sets the currently active shader on the given render context.
  ///
  /// This function has no effect until the next draw or dispatch call on the context.
  void BindShader(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags = xiiShaderBindFlags::Default);

  void                   BindMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void                   BindMeshBuffer(const xiiMeshBufferResourceHandle& hMeshBuffer);
  void                   BindMeshBuffer(xiiGALBufferHandle hVertexBuffer, xiiGALBufferHandle hIndexBuffer, const xiiInputLayoutInfo* pInputLayoutInfo, xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount, xiiGALBufferHandle hVertexBuffer2 = {}, xiiGALBufferHandle hVertexBuffer3 = {}, xiiGALBufferHandle hVertexBuffer4 = {});
  XII_ALWAYS_INLINE void BindNullMeshBuffer(xiiEnum<xiiGALPrimitiveTopology> topology, xiiUInt32 uiPrimitiveCount)
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
  void SetDefaultTextureFilter(xiiEnum<xiiTextureFilterSetting> filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  xiiEnum<xiiTextureFilterSetting> GetDefaultTextureFilter() const { return m_DefaultTextureFilter; }

  /// \brief Returns the 'fixed' texture filter setting that the combination of default texture filter and given \a configuration defines.
  ///
  /// If \a configuration is set to a fixed filter, that setting is returned.
  /// If it is one of LowestQuality to HighestQuality, the adjusted default filter is returned.
  /// When the default filter is used (with adjustments), the allowed range is Bilinear to Aniso16x, the Nearest filter is never used.
  xiiEnum<xiiTextureFilterSetting> GetSpecificTextureFilter(xiiEnum<xiiTextureFilterSetting> configuration) const;

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
  static xiiGALSamplerHandle GetDefaultSampler(xiiBitflags<xiiDefaultSamplerFlags> flags);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RendererContext);

  static void LoadBuiltinShader(xiiShaderUtilities::xiiBuiltinShaderType type, xiiShaderUtilities::xiiBuiltinShader& out_shader);
  static void OnEngineShutdown();

private:
  struct RenderPassFrameBufferInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiGALRenderPassHandle  hRenderPass;
    xiiGALFramebufferHandle hFrameBuffer;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const xiiGALRenderTargetSetup& renderTargetSetup);
    static bool      Equal(const xiiGALRenderTargetSetup& a, const xiiGALRenderTargetSetup& b);

    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);
  };

  void GetRenderPassAndFramebuffer(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPassHandle& out_hRenderPass, xiiGALFramebufferHandle& out_hFramebuffer);

  void BeginRenderPass();
  void EndRenderPass();

private:
  Statistics                         m_Statistics;
  xiiBitflags<xiiRenderContextFlags> m_StateFlags;
  xiiShaderResourceHandle            m_hActiveShader;
  xiiGALShaderHandle                 m_hActiveGALShaders[xiiGALShaderType::ENUM_COUNT];

  xiiHashTable<xiiHashedString, xiiHashedString> m_PermutationVariables;
  xiiMaterialResourceHandle                      m_hNewMaterial;
  xiiMaterialResourceHandle                      m_hMaterial;

  xiiShaderPermutationResourceHandle m_hActiveShaderPermutation;

  xiiBitflags<xiiShaderBindFlags> m_ShaderBindFlags;

  xiiGALBufferHandle               m_hVertexBuffers[4];
  xiiGALBufferHandle               m_hIndexBuffer;
  xiiGALInputLayoutHandle          m_hInputLayout;
  const xiiInputLayoutInfo*        m_pInputLayoutInfo = nullptr;
  xiiEnum<xiiGALPrimitiveTopology> m_Topology;
  xiiUInt32                        m_uiMeshBufferPrimitiveCount;
  xiiEnum<xiiTextureFilterSetting> m_DefaultTextureFilter;
  bool                             m_bAllowAsyncShaderLoading;
  bool                             m_bStereoRendering = false;

  struct ResourceBinding
  {
    XII_DECLARE_POD_TYPE();

    enum Enum : xiiUInt8
    {
      Invalid,
      Buffer,
      Texture
    };

    Enum                    m_Type = Invalid;
    xiiGALBufferViewHandle  m_hBufferView;
    xiiGALTextureViewHandle m_hTextureView;
  };

  xiiHashTable<xiiUInt64, ResourceBinding>     m_BoundResources;
  xiiHashTable<xiiUInt64, ResourceBinding>     m_BoundUAVs;
  xiiHashTable<xiiUInt64, xiiGALSamplerHandle> m_BoundSamplers;

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
    xiiUInt32          m_uiInputLayoutHash;

    XII_FORCE_INLINE bool operator<(const ShaderVertexDecl& rhs) const
    {
      if (m_hShader < rhs.m_hShader)
        return true;
      if (rhs.m_hShader < m_hShader)
        return false;
      return m_uiInputLayoutHash < rhs.m_uiInputLayoutHash;
    }

    XII_FORCE_INLINE bool operator==(const ShaderVertexDecl& rhs) const
    {
      return (m_hShader == rhs.m_hShader && m_uiInputLayoutHash == rhs.m_uiInputLayoutHash);
    }
  };

  static xiiResult BuildInputLayout(xiiGALShaderHandle hVertexShader, const xiiInputLayoutInfo& decl, xiiGALInputLayoutHandle& out_Declaration);

  static xiiMap<ShaderVertexDecl, xiiGALInputLayoutHandle> s_GALInputLayouts;

  static xiiMutex                                                              s_ConstantBufferStorageMutex;
  static xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     s_FreeConstantBufferStorage;

  static xiiGALSamplerHandle s_hDefaultSamplers[4];

  xiiHashTable<xiiGALRenderingSetup, xiiGALRenderPassHandle, xiiRenderContext::ResourceCacheHash>    m_RenderPassCache;
  xiiHashTable<xiiGALRenderingSetup, RenderPassFrameBufferInfo, xiiRenderContext::ResourceCacheHash> m_FramebufferCache;

private: // Per Renderer States
  friend RenderingScope;
  friend ComputeScope;

  // Pipeline state description
  // xiiHashTable<xiiGALPipelineStateCreationDescription, xiiGALPipelineStateHandle, ResourceCacheHash> m_CachedPipelineStates;

  // Renderpass and Framebuffer
  bool                    m_bClearSubmitted   = false;
  bool                    m_bRenderPassActive = false;
  xiiGALFramebufferHandle m_hCurrentFramebuffer;
  xiiGALRenderPassHandle  m_hCurrentRenderPass;
  xiiGALRenderingSetup    m_CurrentRenderingSetup = {};

  xiiGALPipelineStateHandle m_hCurrentPipelineState;
  xiiGALCommandQueue*       m_pCommandQueue = nullptr;
  xiiGALCommandList*        m_pCommandList  = nullptr;
  bool                      m_bCompute      = false;

  // Member Functions
  void UploadConstants();

  void                          SetShaderPermutationVariableInternal(const xiiHashedString& sName, const xiiHashedString& sValue);
  void                          BindShaderInternal(const xiiShaderResourceHandle& hShader, xiiBitflags<xiiShaderBindFlags> flags);
  xiiShaderPermutationResource* ApplyShaderState();
  xiiMaterialResource*          ApplyMaterialState();
  void                          ApplyConstantBufferBindings(xiiGALPipelineState* pPipelineState);
  void                          ApplyResourceViewBindings(xiiGALPipelineState* pPipelineState, xiiEnum<xiiGALShaderResourceType> type);
  void                          ApplyUnorderedAccessViewBindings(xiiGALPipelineState* pPipelineState);
  void                          ApplySamplerBindings(xiiGALPipelineState* pPipelineState);
};
