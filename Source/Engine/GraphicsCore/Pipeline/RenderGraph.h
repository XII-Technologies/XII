#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/SharedPtr.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Resource.h>

/// \brief Describes how a render-graph pass uses a resource.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceAccessFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None                 = 0U,
    Read                 = XII_BIT(0),
    Write                = XII_BIT(1),
    RenderTarget         = XII_BIT(2),
    DepthStencilReadOnly = XII_BIT(3),
    DepthStencilWrite    = XII_BIT(4),
    UnorderedAccess      = XII_BIT(5),
    RayTracing           = XII_BIT(6),
    BuildASRead          = XII_BIT(7),
    BuildASWrite         = XII_BIT(8),

    Default = None
  };

  struct Bits
  {
    StorageType Read : 1;
    StorageType Write : 1;
    StorageType RenderTarget : 1;
    StorageType DepthStencilReadOnly : 1;
    StorageType DepthStencilWrite : 1;
    StorageType UnorderedAccess : 1;
    StorageType RayTracing : 1;
    StorageType BuildASRead : 1;
    StorageType BuildASWrite : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderGraphResourceAccessFlags);

/// \brief Declares one input/output dependency of a pass.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceUsage
{
  xiiHashedString                                m_sResourceName;
  xiiBitflags<xiiRenderGraphResourceAccessFlags> m_AccessFlags   = xiiRenderGraphResourceAccessFlags::Read;
  xiiBitflags<xiiGALResourceStateFlags>          m_RequiredState = xiiGALResourceStateFlags::Unknown;
};

/// \brief Immutable pass metadata used by the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphPassDescription
{
  xiiHashedString                                 m_sPassName;
  xiiBitflags<xiiGALCommandQueueFlags>            m_QueueFlags      = xiiGALCommandQueueFlags::Graphics;
  bool                                            m_bHasSideEffects = false;
  xiiHybridArray<xiiRenderGraphResourceUsage, 8U> m_Inputs;
  xiiHybridArray<xiiRenderGraphResourceUsage, 8U> m_Outputs;
};

/// \brief Compile-time knobs for optimizing RenderGraph scheduling.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphCompileSettings
{
  bool      m_bEnablePassCulling  = false;
  bool      m_bEnableCompileCache = true;
  xiiUInt32 m_uiCacheSalt         = 0U;
};

/// \brief Runtime statistics describing the latest compile/execute state.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphStatistics
{
  xiiUInt32 m_uiRegisteredPassCount = 0U;
  xiiUInt32 m_uiCompiledPassCount   = 0U;
  xiiUInt32 m_uiCulledPassCount     = 0U;
  xiiUInt32 m_uiBarrierCount        = 0U;
  xiiUInt64 m_uiGraphSignature      = 0ULL;
  bool      m_bUsedCachedCompile    = false;
};

/// \brief Context provided to a pass when recording commands.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphPassExecutionContext
{
  xiiGALCommandList* m_pCommandList = nullptr;
  const xiiView*     m_pView        = nullptr;
  const xiiViewData* m_pViewData    = nullptr;
};

/// \brief Interface implemented by all v2 render-graph passes.
class XII_GRAPHICSCORE_DLL xiiRenderGraphPassBase
{
public:
  virtual ~xiiRenderGraphPassBase() = default;

  [[nodiscard]] virtual const xiiRenderGraphPassDescription& GetDescription() const = 0;

  virtual void RecordCommands(const xiiRenderGraphPassExecutionContext& executionContext) const = 0;
};

/// \brief Compiled pass metadata used during submission scheduling.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphCompiledPass
{
  const xiiRenderGraphPassBase* m_pPass       = nullptr;
  xiiUInt32                     m_uiPassIndex = 0;
  xiiHybridArray<xiiUInt32, 8U> m_Dependencies;
};

/// \brief Represents one synthesized resource state transition between two passes.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphBarrier
{
  xiiHashedString                       m_sResourceName;
  xiiBitflags<xiiGALResourceStateFlags> m_BeforeState     = xiiGALResourceStateFlags::Unknown;
  xiiBitflags<xiiGALResourceStateFlags> m_AfterState      = xiiGALResourceStateFlags::Unknown;
  xiiUInt32                             m_uiFromPassIndex = xiiInvalidIndex;
  xiiUInt32                             m_uiToPassIndex   = xiiInvalidIndex;
};

/// \brief Resolves graph resource names to runtime GAL resources during execution.
class XII_GRAPHICSCORE_DLL xiiRenderGraphResourceResolver
{
public:
  virtual ~xiiRenderGraphResourceResolver() = default;

  [[nodiscard]] virtual xiiSharedPtr<xiiGALResource> ResolveResource(xiiHashedString sResourceName) const = 0;
};

/// \brief Minimal graph compiler for the new explicit rendering path.
///
/// This implementation validates pass descriptions and compiles passes into deterministic topological order.
/// Barrier synthesis is intentionally deferred to the next iteration.
class XII_GRAPHICSCORE_DLL xiiRenderGraphCompiler
{
public:
  void AddPass(const xiiRenderGraphPassBase* pPass);
  void Reset();

  [[nodiscard]] xiiResult Compile(xiiDynamicArray<xiiRenderGraphCompiledPass>& out_compiledPasses, xiiDynamicArray<xiiRenderGraphBarrier>& out_barriers, const xiiRenderGraphCompileSettings& compileSettings, xiiRenderGraphStatistics* out_pStatistics, xiiStringBuilder* out_pErrorMessage = nullptr) const;

private:
  [[nodiscard]] xiiResult                                    ValidatePassDescription(const xiiRenderGraphPassDescription& passDescription, xiiUInt32 uiPassIndex, xiiStringBuilder* out_pErrorMessage) const;
  [[nodiscard]] static xiiBitflags<xiiGALResourceStateFlags> DeriveRequiredState(xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags);

private:
  xiiDynamicArray<const xiiRenderGraphPassBase*> m_Passes;
};

/// \brief Executes a compiled render graph and applies synthesized barriers.
class XII_GRAPHICSCORE_DLL xiiRenderGraphExecutor
{
public:
  [[nodiscard]] xiiResult Execute(const xiiArrayPtr<const xiiRenderGraphCompiledPass> compiledPasses, const xiiArrayPtr<const xiiRenderGraphBarrier> barriers, const xiiRenderGraphPassExecutionContext& executionContext, const xiiRenderGraphResourceResolver* pResourceResolver = nullptr, xiiStringBuilder* out_pErrorMessage = nullptr) const;
};

/// \brief Default resource table used by xiiRenderGraphRuntime.
class XII_GRAPHICSCORE_DLL xiiRenderGraphResourceTable : public xiiRenderGraphResourceResolver
{
public:
  XII_ALWAYS_INLINE void SetResource(xiiHashedString sResourceName, xiiSharedPtr<xiiGALResource> pResource)
  {
    if (pResource == nullptr)
    {
      m_Resources.Remove(sResourceName);
      return;
    }

    m_Resources.Insert(sResourceName, pResource);
  }

  XII_ALWAYS_INLINE void RemoveResource(xiiHashedString sResourceName)
  {
    m_Resources.Remove(sResourceName);
  }

  XII_ALWAYS_INLINE void ClearResources()
  {
    m_Resources.Clear();
  }

  [[nodiscard]] XII_ALWAYS_INLINE virtual xiiSharedPtr<xiiGALResource> ResolveResource(xiiHashedString sResourceName) const override
  {
    xiiSharedPtr<xiiGALResource> pResource;
    if (m_Resources.TryGetValue(sResourceName, pResource))
    {
      return pResource;
    }

    return {};
  }

private:
  xiiHashTable<xiiHashedString, xiiSharedPtr<xiiGALResource>> m_Resources;
};

/// \brief Bootstrap runtime that owns pass registration, compilation, and execution for RenderGraph v2.
class XII_GRAPHICSCORE_DLL xiiRenderGraphRuntime
{
public:
  void AddPass(const xiiRenderGraphPassBase* pPass);
  void ClearPasses();

  XII_ALWAYS_INLINE void SetCompileSettings(const xiiRenderGraphCompileSettings& compileSettings)
  {
    m_CompileSettings = compileSettings;
    m_bIsCompiled     = false;
  }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiRenderGraphCompileSettings& GetCompileSettings() const { return m_CompileSettings; }

  void                   SetExternalResourceResolver(const xiiRenderGraphResourceResolver* pResourceResolver);
  XII_ALWAYS_INLINE void SetResource(xiiHashedString sResourceName, xiiSharedPtr<xiiGALResource> pResource)
  {
    m_LocalResources.SetResource(sResourceName, pResource);
  }
  void RemoveResource(xiiHashedString sResourceName);
  void ClearResources();

  [[nodiscard]] xiiResult Compile(xiiStringBuilder* out_pErrorMessage = nullptr);
  [[nodiscard]] xiiResult Execute(const xiiRenderGraphPassExecutionContext& executionContext, xiiStringBuilder* out_pErrorMessage = nullptr) const;

  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphCompiledPass> GetCompiledPasses() const { return m_CompiledPasses; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphBarrier> GetBarriers() const { return m_Barriers; }
  [[nodiscard]] XII_ALWAYS_INLINE const xiiRenderGraphStatistics&          GetStatistics() const { return m_Statistics; }

private:
  xiiDynamicArray<const xiiRenderGraphPassBase*> m_Passes;

  xiiRenderGraphCompileSettings m_CompileSettings;

  xiiRenderGraphCompiler      m_Compiler;
  xiiRenderGraphExecutor      m_Executor;
  xiiRenderGraphResourceTable m_LocalResources;

  xiiDynamicArray<xiiRenderGraphCompiledPass> m_CompiledPasses;
  xiiDynamicArray<xiiRenderGraphBarrier>      m_Barriers;

  xiiRenderGraphStatistics m_Statistics;
  xiiUInt64                m_uiLastCompileSignature = 0ULL;

  const xiiRenderGraphResourceResolver* m_pExternalResourceResolver = nullptr;
  bool                                  m_bIsCompiled               = false;
};
