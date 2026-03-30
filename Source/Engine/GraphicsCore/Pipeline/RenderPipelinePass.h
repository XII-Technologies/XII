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
  xiiRenderPipelinePass(xiiStringView sName, bool bActive = true);
  virtual ~xiiRenderPipelinePass();

  /// \brief Called each frame when the world module prepares the render graph for a given view.
  virtual void AddToGraph(xiiView& view, xiiRenderGraph& graph, class xiiRenderGraphBlackboard& blackboard) = 0;

  xiiStringView GetName() const { return m_sName; }

  void SetActive(bool bActive) { m_bActive = bActive; }
  bool IsActive() const { return m_bActive; }

protected:
  xiiString m_sName;
  bool      m_bActive = true;
};
