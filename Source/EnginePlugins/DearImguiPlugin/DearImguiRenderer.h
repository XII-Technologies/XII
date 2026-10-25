/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <Imgui/imgui.h>

class xiiImguiTexture;

struct xiiImguiDrawCommand
{
  xiiVec4   m_vClipRect;
  xiiUInt32 m_uiFirstIndex = 0;
  xiiUInt32 m_uiBaseVertex = 0;
  xiiUInt32 m_uiIndexCount = 0;
  xiiUInt32 m_uiTexture    = 0;
};

/// Deep copy of finalized draw data, with owned texture versions. Contains no borrowed ImGui or view pointers.
/// Published only as const shared ownership. May outlive its context and be bound to several related views.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiFrame : public xiiRefCounted
{
public:
  xiiImguiFrame();
  ~xiiImguiFrame();
  xiiArrayPtr<const ImDrawVert>                    GetVertices() const { return m_Vertices; }
  xiiArrayPtr<const ImDrawIdx>                     GetIndices() const { return m_Indices; }
  xiiArrayPtr<const xiiImguiDrawCommand>           GetCommands() const { return m_Commands; }
  const xiiVec4&                                   GetDisplayRect() const { return m_vDisplayRect; }
  xiiArrayPtr<const xiiSharedPtr<xiiImguiTexture>> GetTextures() const { return m_Textures; }

private:
  friend class xiiImguiContext;
  xiiVec4                                        m_vDisplayRect = xiiVec4::MakeZero();
  xiiDynamicArray<ImDrawVert>                    m_Vertices;
  xiiDynamicArray<ImDrawIdx>                     m_Indices;
  xiiDynamicArray<xiiImguiDrawCommand>           m_Commands;
  xiiDynamicArray<xiiSharedPtr<xiiImguiTexture>> m_Textures;
};

/// Setup-local binding of a reusable UI snapshot. Create a new binding for each graph setup serial.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiFrameData final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiImguiFrameData, xiiRenderPipelineFrameData);

public:
  xiiImguiFrameData(xiiUInt64 uiSetupSerial, xiiSharedPtr<const xiiImguiFrame> pFrame);
  const xiiSharedPtr<const xiiImguiFrame> m_pFrame;
};
