#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

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
    XII_ASSERT_DEV(!m_bGroupDuplicates, "Not yet implemented");

    xiiAbstractObjectGraph graph;
    DeserializeGraph(graph);

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
        rngRotX.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds(), (xiiUInt32)(m_vRandomRotation.x));
      if (m_vRandomRotation.y > 0)
        rngRotY.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 1, (xiiUInt32)(m_vRandomRotation.y));
      if (m_vRandomRotation.z > 0)
        rngRotZ.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 2, (xiiUInt32)(m_vRandomRotation.z));

      if (m_vRandomTranslation.x > 0)
        rngTransX.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 3, (xiiUInt32)(m_vRandomTranslation.x * 100));
      if (m_vRandomTranslation.y > 0)
        rngTransY.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 4, (xiiUInt32)(m_vRandomTranslation.y * 100));
      if (m_vRandomTranslation.z > 0)
        rngTransZ.Initialize((xiiUInt64)xiiTime::Now().GetNanoseconds() + 5, (xiiUInt32)(m_vRandomTranslation.z * 100));

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

  return xiiStatus(XII_SUCCESS);
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

void xiiDuplicateObjectsCommand::DeserializeGraph(xiiAbstractObjectGraph& graph)
{
  xiiRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
  xiiAbstractGraphDdlSerializer::Read(memoryReader, &graph).IgnoreResult();
}

void xiiDuplicateObjectsCommand::CreateOneDuplicate(xiiAbstractObjectGraph& graph, xiiHybridArray<xiiDocument::PasteInfo, 16>& ToBePasted)
{
  xiiSceneDocument* pDocument = static_cast<xiiSceneDocument*>(GetDocument());

  // Remap
  const xiiUuid seed = xiiUuid::MakeUuid();
  graph.ReMapNodeGuids(seed);

  xiiDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateOnly);


  xiiStringBuilder sParentGuids = m_sParentNodes;
  xiiStringBuilder sNextParentGuid;

  xiiMap<xiiUuid, xiiUuid> ParentGuids;

  while (!sParentGuids.IsEmpty())
  {
    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    xiiUuid guidObj = xiiConversionUtils::ConvertStringToUuid(sNextParentGuid);
    guidObj.CombineWithSeed(seed);

    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    ParentGuids[guidObj] = xiiConversionUtils::ConvertStringToUuid(sNextParentGuid);
  }

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "root")
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref     = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = nullptr;

        const xiiUuid guidParent = ParentGuids[pNode->GetGuid()];

        if (guidParent.IsValid())
          ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
      }
    }
  }

  if (pDocument->DuplicateSelectedObjects(ToBePasted, graph, false))
  {
    for (const auto& item : ToBePasted)
    {
      auto& po             = m_DuplicatedObjects.ExpandAndGetRef();
      po.m_pObject         = item.m_pObject;
      po.m_Index           = item.m_pObject->GetPropertyIndex();
      po.m_pParent         = item.m_pParent;
      po.m_sParentProperty = item.m_pObject->GetParentProperty();
    }
  }
  else
  {
    for (const auto& item : ToBePasted)
    {
      pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
    }
  }

  // undo uuid changes, so that we can do this again with another seed
  graph.ReMapNodeGuids(seed, true);
}


void xiiDuplicateObjectsCommand::AdjustObjectPositions(xiiHybridArray<xiiDocument::PasteInfo, 16>& Duplicates, xiiUInt32 uiNumDuplicate, xiiRandomGauss& rngRotX, xiiRandomGauss& rngRotY, xiiRandomGauss& rngRotZ, xiiRandomGauss& rngTransX, xiiRandomGauss& rngTransY, xiiRandomGauss& rngTransZ)
{
  xiiSceneDocument* pScene = static_cast<xiiSceneDocument*>(m_pDocument);

  const float fStep = uiNumDuplicate;

  xiiVec3 vRandT(0.0f);
  xiiVec3 vRandR(0.0f);

  if (m_vRandomRotation.x != 0)
    vRandR.x = rngRotX.SignedValue();
  if (m_vRandomRotation.y != 0)
    vRandR.y = rngRotY.SignedValue();
  if (m_vRandomRotation.z != 0)
    vRandR.z = rngRotZ.SignedValue();

  if (m_vRandomTranslation.x != 0)
    vRandT.x = rngTransX.SignedValue() / 100.0f;
  if (m_vRandomTranslation.y != 0)
    vRandT.y = rngTransY.SignedValue() / 100.0f;
  if (m_vRandomTranslation.z != 0)
    vRandT.z = rngTransZ.SignedValue() / 100.0f;

  xiiVec3 vPosOffset(0.0f);

  if (m_iRevolveAxis > 0 && m_fRevolveRadius != 0.0f && m_RevolveAngleStep != xiiAngle())
  {
    xiiVec3  vRevolveAxis(0.0f);
    xiiAngle revolve = m_RevolveStartAngle;

    switch (m_iRevolveAxis)
    {
      case 1:
        vRevolveAxis.Set(1, 0, 0);
        vPosOffset.Set(0, 0, m_fRevolveRadius);
        break;
      case 2:
        vRevolveAxis.Set(0, 1, 0);
        vPosOffset.Set(m_fRevolveRadius, 0, 0);
        break;
      case 3:
        vRevolveAxis.Set(0, 0, 1);
        vPosOffset.Set(0, m_fRevolveRadius, 0);
        break;
    }

    revolve += fStep * m_RevolveAngleStep;

    xiiMat3 mRevolve = xiiMat3::MakeAxisRotation(vRevolveAxis, revolve);

    vPosOffset = mRevolve * vPosOffset;
  }

  xiiQuat qRot = xiiQuat::MakeFromEulerAngles(xiiAngle::Degree(fStep * m_vAccumulativeRotation.x + vRandR.x), xiiAngle::Degree(fStep * m_vAccumulativeRotation.y + vRandR.y), xiiAngle::Degree(fStep * m_vAccumulativeRotation.z + vRandR.z));

  for (const auto& pi : Duplicates)
  {
    xiiTransform tGlobal = pScene->GetGlobalTransform(pi.m_pObject);

    tGlobal.m_vScale.Set(1.0f);
    tGlobal.m_vPosition += vPosOffset + (1.0f + fStep) * m_vAccumulativeTranslation + vRandT;
    tGlobal.m_qRotation = qRot * tGlobal.m_qRotation;

    /// \todo Christopher: Modifying the position through a command after creating the object seems to destroy the undo-ability of this
    /// operation Duplicating multiple objects (with some translation) and then undoing that will crash the editor process

    pScene->SetGlobalTransform(pi.m_pObject, tGlobal, TransformationChanges::Translation | TransformationChanges::Rotation);
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

  return xiiStatus(XII_SUCCESS);
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
