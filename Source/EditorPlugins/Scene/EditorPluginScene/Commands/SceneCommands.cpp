/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDuplicateObjectsCommand, 1, xiiRTTIDefaultAllocator<xiiDuplicateObjectsCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GraphText", m_sGraphTextFormat),
    XII_MEMBER_PROPERTY("ParentNodes", m_sParentNodes),
    XII_MEMBER_PROPERTY("NumCopies", m_uiNumberOfCopies),
    XII_MEMBER_PROPERTY("Translate", m_vAccumulativeTranslation),
    XII_MEMBER_PROPERTY("Rotate", m_vAccumulativeRotation),
    XII_MEMBER_PROPERTY("RandomRotation", m_vRandomRotation),
    XII_MEMBER_PROPERTY("RandomTranslation", m_vRandomTranslation),
    XII_MEMBER_PROPERTY("Group", m_bGroupDuplicates),
    XII_MEMBER_PROPERTY("RevolveAxis", m_iRevolveAxis),
    XII_MEMBER_PROPERTY("RevoleStartAngle", m_RevolveStartAngle),
    XII_MEMBER_PROPERTY("RevolveAngleStep", m_RevolveAngleStep),
    XII_MEMBER_PROPERTY("RevolveRadius", m_fRevolveRadius),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


////////////////////////////////////////////////////////////////////////
// xiiDuplicateObjectsCommand
////////////////////////////////////////////////////////////////////////

xiiDuplicateObjectsCommand::xiiDuplicateObjectsCommand()
{
  m_uiNumberOfCopies = 0;
  m_vAccumulativeTranslation.SetZero();
  m_vAccumulativeRotation.SetZero();
  m_vRandomRotation.SetZero();
  m_vRandomTranslation.SetZero();
  m_iRevolveAxis     = 0;
  m_fRevolveRadius   = 0.0f;
  m_bGroupDuplicates = false;
}

xiiStatus xiiDuplicateObjectsCommand::DoInternal(bool bRedo)
{
  xiiSceneDocument* pDocument = static_cast<xiiSceneDocument*>(GetDocument());

  if (!bRedo)
  {
    if (!m_vAccumulativeTranslation.IsValid() || !m_vAccumulativeRotation.IsValid() || !m_vRandomRotation.IsValid() || !m_vRandomTranslation.IsValid() || m_vRandomRotation.CompMin(xiiVec3::MakeZero()) != xiiVec3::MakeZero() || m_vRandomTranslation.CompMin(xiiVec3::MakeZero()) != xiiVec3::MakeZero() || !xiiMath::IsFinite(m_fRevolveRadius) || m_fRevolveRadius < 0 || !xiiMath::IsFinite(m_RevolveStartAngle.GetRadian()) || !xiiMath::IsFinite(m_RevolveAngleStep.GetRadian()) || m_iRevolveAxis < 0 || m_iRevolveAxis > 3)
      return xiiStatus("Duplicate Objects: transforms must be finite, random ranges and radius nonnegative, and revolve axis valid.");

    xiiAbstractObjectGraph graph;
    XII_SUCCEED_OR_RETURN(DeserializeGraph(graph));
    if (m_bGroupDuplicates)
      XII_SUCCEED_OR_RETURN(GroupRootObjects(graph));

    if (m_uiNumberOfCopies == 0)
    {
      xiiHybridArray<xiiDocument::PasteInfo, 16> ToBePasted;
      CreateOneDuplicate(graph, ToBePasted);
    }
    else
    {
      // store original selection
      m_OriginalSelection = m_pDocument->GetSelectionManager()->GetSelection();

      xiiHybridArray<xiiHybridArray<xiiDocument::PasteInfo, 16>, 8> ToBePasted;
      ToBePasted.SetCount(m_uiNumberOfCopies);

      for (xiiUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        CreateOneDuplicate(graph, ToBePasted[copies]);
      }

      xiiRandomGauss rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ;

      if (m_vRandomRotation.x > 0)
        rngRotX.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds(), 1024);
      if (m_vRandomRotation.y > 0)
        rngRotY.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 1, 1024);
      if (m_vRandomRotation.z > 0)
        rngRotZ.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 2, 1024);

      if (m_vRandomTranslation.x > 0)
        rngTransX.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 3, 1024);
      if (m_vRandomTranslation.y > 0)
        rngTransY.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 4, 1024);
      if (m_vRandomTranslation.z > 0)
        rngTransZ.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 5, 1024);

      for (xiiUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        AdjustObjectPositions(ToBePasted[copies], copies, rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ);
      }
    }


    if (m_DuplicatedObjects.IsEmpty())
      return xiiStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_DuplicatedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  SetAsSelection();

  return XII_SUCCESS;
}

void xiiDuplicateObjectsCommand::SetAsSelection()
{
  if (!m_DuplicatedObjects.IsEmpty())
  {
    auto pSelMan = m_pDocument->GetSelectionManager();

    xiiDeque<const xiiDocumentObject*> NewSelection = m_OriginalSelection;

    for (const DuplicatedObject& pi : m_DuplicatedObjects)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }
}

xiiStatus xiiDuplicateObjectsCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  xiiDocument* pDocument = GetDocument();

  for (auto& po : m_DuplicatedObjects)
  {
    XII_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return XII_SUCCESS;
}

void xiiDuplicateObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_DuplicatedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_DuplicatedObjects.Clear();
  }
}
