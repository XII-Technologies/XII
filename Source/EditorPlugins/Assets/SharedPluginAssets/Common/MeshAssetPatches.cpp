/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

namespace
{
  class MeshPackedLayoutPatch : public xiiGraphPatch
  {
  public:
    MeshPackedLayoutPatch(const char* szType, xiiUInt32 uiVersion) : xiiGraphPatch(szType, uiVersion) {}

    void Patch(xiiGraphPatchContext&, xiiAbstractObjectGraph*, xiiAbstractObjectNode* pNode) const override
    {
      // Native packed meshes have one authoritative layout. Rebuilding from the source
      // retains float normals/UVs and uses native byte bone weights; obsolete precision
      // choices cannot configure that layout and must not remain as inactive UI settings.
      pNode->RemoveProperty("NormalPrecision");
      pNode->RemoveProperty("TexCoordPrecision");
      pNode->RemoveProperty("BoneWeightPrecision");
      if (auto* property = pNode->FindProperty("VertexColorConversion"); property != nullptr && property->m_Value.IsA<xiiString>())
      {
        const auto& value = property->m_Value.Get<xiiString>();
        if (value == "xiiMeshVertexColorConversion::None")
          property->m_Value = "xiiModelVertexColorConversion::None";
        else if (value == "xiiMeshVertexColorConversion::LinearToSrgb")
          property->m_Value = "xiiModelVertexColorConversion::LinearToSRGB";
        else if (value == "xiiMeshVertexColorConversion::SrgbToLinear")
          property->m_Value = "xiiModelVertexColorConversion::SRGBToLinear";
      }
      // Numeric conversion values retain their original identities (0, 1, 2).
    }
  };

  MeshPackedLayoutPatch s_StaticMeshPatch("xiiMeshAssetProperties", 5);
  MeshPackedLayoutPatch s_AnimatedMeshPatch("xiiAnimatedMeshAssetProperties", 3);
} // namespace
