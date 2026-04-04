#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiView;

/// \brief Shared pass state for concrete render-pass implementations.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePass, xiiReflectedClass);

public:
  /// \brief Sets the name of this pass.
  XII_ALWAYS_INLINE xiiStringView GetName() const { return m_sName; }

  /// \brief Sets whether this pass is active. Inactive passes are skipped when building the render graph.
  XII_ALWAYS_INLINE void SetActive(bool bActive) { m_bActive = bActive; }

  /// \brief Returns whether this pass is active. Inactive passes are skipped when building the render graph.
  XII_ALWAYS_INLINE bool IsActive() const { return m_bActive; }

public:
  xiiRenderPipelinePass(xiiStringView sName, bool bActive = true);

  virtual ~xiiRenderPipelinePass();

protected:
  xiiString m_sName;
  bool      m_bActive = true;
};
