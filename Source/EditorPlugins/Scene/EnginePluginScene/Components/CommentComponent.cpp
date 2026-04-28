/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/CommentComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiCommentComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Comment", GetComment, SetComment),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Editing"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCommentComponent::xiiCommentComponent()  = default;
xiiCommentComponent::~xiiCommentComponent() = default;

void xiiCommentComponent::SetComment(const char* szText)
{
  m_sComment.Assign(szText);
}

const char* xiiCommentComponent::GetComment() const
{
  return m_sComment.GetString();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneExportModifier_RemoveCommentComponents, 1, xiiRTTIDefaultAllocator<xiiSceneExportModifier_RemoveCommentComponents>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneExportModifier_RemoveCommentComponents::ModifyWorld(xiiWorld& ref_world, xiiStringView sDocumentType, const xiiUuid& documentGuid, bool bForExport)
{
  XII_LOCK(ref_world.GetWriteMarker());

  if (xiiCommentComponentManager* pMan = ref_world.GetComponentManager<xiiCommentComponentManager>())
  {
    for (auto it = pMan->GetComponents(); it.IsValid(); it.Next())
    {
      pMan->DeleteComponent(it);
    }
  }
}
