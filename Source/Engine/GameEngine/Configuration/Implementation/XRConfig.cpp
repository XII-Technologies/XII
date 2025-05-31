#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Configuration/XRConfig.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiXRConfig, 2, xiiRTTIDefaultAllocator<xiiXRConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EnableXR", m_bEnableXR),
    // HololensRenderPipeline.xiiRenderPipelineAsset
    XII_MEMBER_PROPERTY("XRRenderPipeline", m_sXRRenderPipeline)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new xiiDefaultValueAttribute(xiiStringView("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"))),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiXRConfig::SaveRuntimeData(xiiChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("xiiXRConfig", 2);

  inout_stream << m_bEnableXR;
  inout_stream << m_sXRRenderPipeline;

  inout_stream.EndChunk();
}

void xiiXRConfig::LoadRuntimeData(xiiChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "xiiVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "xiiXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class xiiVRConfig_1_2 : public xiiGraphPatch
{
public:
  xiiVRConfig_1_2() :
    xiiGraphPatch("xiiVRConfig", 5)
  {
  }
  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("xiiXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

xiiVRConfig_1_2 g_xiiVRConfig_1_2;

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
