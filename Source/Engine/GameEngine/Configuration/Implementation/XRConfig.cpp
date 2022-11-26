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

void xiiXRConfig::SaveRuntimeData(xiiChunkStreamWriter& stream) const
{
  stream.BeginChunk("xiiXRConfig", 2);

  stream << m_bEnableXR;
  stream << m_sXRRenderPipeline;

  stream.EndChunk();
}

void xiiXRConfig::LoadRuntimeData(xiiChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "xiiVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    stream >> m_bEnableXR;
    stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "xiiXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    stream >> m_bEnableXR;
    stream >> m_sXRRenderPipeline;
  }
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class xiiVRConfig_1_2 : public xiiGraphPatch
{
public:
  xiiVRConfig_1_2() :
    xiiGraphPatch("xiiVRConfig", 5)
  {
  }
  virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    context.RenameClass("xiiXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

xiiVRConfig_1_2 g_xiiVRConfig_1_2;

XII_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
