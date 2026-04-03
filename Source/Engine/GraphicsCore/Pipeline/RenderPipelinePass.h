#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class xiiRenderGraph;
class xiiView;

/// \brief Base class for pipeline passes that are instantiated and register themselves with the render graph.
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

  /// \brief Called each frame when the world module prepares the render graph for a given view.
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, class xiiRenderGraphBlackboard& blackboard) = 0;

protected:
  xiiString m_sName;
  bool      m_bActive = true;
};
