#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/StringBuilder.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

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
    RayTracingStructure  = XII_BIT(6),

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
    StorageType RayTracingStructure : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderGraphResourceAccessFlags);

/// \brief Declares one input/output dependency of a pass.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphResourceUsage
{
  xiiHashedString                                 m_sResourceName;
  xiiBitflags<xiiRenderGraphResourceAccessFlags> m_AccessFlags   = xiiRenderGraphResourceAccessFlags::Read;
  xiiBitflags<xiiGALResourceStateFlags>          m_RequiredState = xiiGALResourceStateFlags::Unknown;
};

/// \brief Immutable pass metadata used by the render graph compiler.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphPassDescription
{
  xiiHashedString                        m_sPassName;
  xiiBitflags<xiiGALCommandQueueFlags>  m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
  xiiHybridArray<xiiRenderGraphResourceUsage, 8U> m_Inputs;
  xiiHybridArray<xiiRenderGraphResourceUsage, 8U> m_Outputs;
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

/// \brief Minimal graph compiler for the new explicit rendering path.
///
/// This implementation validates pass descriptions and compiles passes into deterministic topological order.
/// Barrier synthesis is intentionally deferred to the next iteration.
class XII_GRAPHICSCORE_DLL xiiRenderGraphCompiler
{
public:
  void AddPass(const xiiRenderGraphPassBase* pPass);
  void Reset();

  [[nodiscard]] xiiResult Compile(xiiDynamicArray<xiiRenderGraphCompiledPass>& out_compiledPasses, xiiStringBuilder* out_pErrorMessage = nullptr) const;

private:
  [[nodiscard]] xiiResult ValidatePassDescription(const xiiRenderGraphPassDescription& passDescription, xiiUInt32 uiPassIndex, xiiStringBuilder* out_pErrorMessage) const;
  [[nodiscard]] static xiiBitflags<xiiGALResourceStateFlags> DeriveRequiredState(xiiBitflags<xiiRenderGraphResourceAccessFlags> accessFlags);

private:
  xiiDynamicArray<const xiiRenderGraphPassBase*> m_Passes;
};
