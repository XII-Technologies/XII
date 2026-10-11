/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <SharedPluginAssets/Common/MeshMaterialSlots.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MeshMaterialSlots)
{
  xiiHybridArray<xiiMaterialResourceSlot, 8> slots;
  slots.PushBack({"Metal", "Metal.material", true});
  slots.PushBack({"Paint", "PaintA.material", false});
  slots.PushBack({"Paint", "PaintB.material", true});
  slots.PushBack({"", "Unnamed.material", false});
  slots.PushBack({"Removed", "Removed.material", false});
  const xiiStringView labels[] = {"Paint", "Metal", "Paint", "New", "Paint"};
  xiiMeshMaterialSlots::Reconcile(labels, slots);
  XII_TEST_INT(slots.GetCount(), 5);
  XII_TEST_STRING(slots[0].m_sResource.GetData(), "PaintA.material");
  XII_TEST_STRING(slots[1].m_sResource.GetData(), "Metal.material");
  XII_TEST_STRING(slots[2].m_sResource.GetData(), "PaintB.material");
  XII_TEST_STRING(slots[3].m_sResource.GetData(), "Unnamed.material");
  XII_TEST_BOOL(slots[4].m_sResource.IsEmpty());
  XII_TEST_BOOL(!slots[0].m_bHighlight && slots[1].m_bHighlight && slots[2].m_bHighlight);
  const auto copy = slots;
  xiiMeshMaterialSlots::Reconcile(labels, slots);
  XII_TEST_BOOL(xiiMeshMaterialSlots::Equal(slots, copy));
  slots[0].m_sResource = "UserOverride.material";
  XII_TEST_BOOL(!xiiMeshMaterialSlots::Equal(slots, copy));
  const xiiStringView shared[] = {"Paint"};
  xiiMeshMaterialSlots::Reconcile(shared, slots);
  XII_TEST_INT(slots.GetCount(), 1);
  XII_TEST_STRING(slots[0].m_sResource.GetData(), "UserOverride.material");
  xiiMeshMaterialSlots::Reconcile({}, slots);
  XII_TEST_BOOL(slots.IsEmpty());
  const xiiStringView unnamed[] = {"", ""};
  xiiMeshMaterialSlots::Reconcile(unnamed, slots);
  slots[0].m_sResource = "First.material";
  slots[1].m_sResource = "Second.material";
  xiiMeshMaterialSlots::Reconcile(unnamed, slots);
  XII_TEST_STRING(slots[0].m_sResource.GetData(), "First.material");
  XII_TEST_STRING(slots[1].m_sResource.GetData(), "Second.material");
}
