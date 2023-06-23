#include <EditorPluginScene/EditorPluginScenePCH.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class xiiFakeRopeComponentPatch_2_3 : public xiiGraphPatch
{
public:
  xiiFakeRopeComponentPatch_2_3() :
    xiiGraphPatch("xiiFakeRopeComponent", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

xiiFakeRopeComponentPatch_2_3 g_xiiFakeRopeComponentPatch_2_3;
