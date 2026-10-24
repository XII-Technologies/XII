/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GuiFoundation/NodeEditor/NodeLayout.h>
#include <initializer_list>

namespace
{
  template <typename T>
  void Assign(xiiDynamicArray<T>& destination, std::initializer_list<T> values)
  {
    destination = xiiArrayPtr<const T>(values.begin(), static_cast<xiiUInt32>(values.size()));
  }
  void CheckNoOverlap(xiiArrayPtr<const QRectF> bounds, xiiArrayPtr<const xiiVec2> positions)
  {
    for (xiiUInt32 i = 0; i < bounds.GetCount(); ++i)
      for (xiiUInt32 j = 0; j < i; ++j)
        XII_TEST_BOOL(!QRectF(QPointF(positions[i].x, positions[i].y), bounds[i].size()).intersects(QRectF(QPointF(positions[j].x, positions[j].y), bounds[j].size())));
  }

  void CheckRepeat(xiiDynamicArray<QRectF> bounds, xiiArrayPtr<const xiiNodeLayoutConnection> connections, xiiArrayPtr<const xiiVec2> positions)
  {
    for (xiiUInt32 i = 0; i < bounds.GetCount(); ++i)
      bounds[i].moveTopLeft(QPointF(positions[i].x, positions[i].y));
    xiiDynamicArray<xiiVec2> repeated;
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, connections, repeated));
    for (xiiUInt32 i = 0; i < positions.GetCount(); ++i)
      XII_TEST_BOOL(positions[i].IsEqual(repeated[i], 0.01f));
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(NodeLayout);

XII_CREATE_SIMPLE_TEST(NodeLayout, TopologyAndGeometry)
{
  xiiDynamicArray<QRectF>                  bounds;
  xiiDynamicArray<xiiNodeLayoutConnection> edges;
  xiiDynamicArray<xiiVec2>                 positions;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Variable-size chain aligns pins and flows forward")
  {
    Assign(bounds, {QRectF(90, -40, 250, 300), QRectF(-80, 70, 100, 80), QRectF(10, 10, 400, 130)});
    Assign(edges, {{0, 1, 230, 25}, {1, 2, 25, 85}});
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, edges, positions));
    CheckNoOverlap(bounds, positions);
    XII_TEST_BOOL(positions[0].x + bounds[0].width() < positions[1].x);
    XII_TEST_BOOL(positions[1].x + bounds[1].width() < positions[2].x);
    XII_TEST_FLOAT(positions[0].y + 230, positions[1].y + 25, 0.01);
    XII_TEST_FLOAT(positions[1].y + 25, positions[2].y + 85, 0.01);
    CheckRepeat(bounds, edges, positions);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Crossed branches reorder, merge and retain deterministic spacing")
  {
    bounds.SetCount(5, QRectF(0, 0, 180, 90));
    Assign(edges, {{0, 3, 50, 50}, {1, 2, 50, 50}, {2, 4, 50, 75}, {3, 4, 50, 25}});
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, edges, positions));
    CheckNoOverlap(bounds, positions);
    XII_TEST_BOOL((positions[0].y < positions[1].y) == (positions[3].y < positions[2].y));
    CheckRepeat(bounds, edges, positions);
    xiiDynamicArray<xiiVec2> repeat;
    for (auto& rect : bounds)
      rect.translate(100, -200);
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, edges, repeat));
    for (xiiUInt32 i = 0; i < positions.GetCount(); ++i)
      XII_TEST_BOOL((positions[i] + xiiVec2(100, -200)).IsEqual(repeat[i], 0.01f));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cycles, self loops, parallel wires and isolated components")
  {
    Assign(bounds, {QRectF(0, 0, 120, 90), QRectF(0, 0, 300, 200), QRectF(0, 0, 120, 90), QRectF(0, 0, 180, 150), QRectF(0, 0, 250, 50), QRectF(0, 0, 60, 60)});
    Assign(edges, {{0, 1, 40, 40}, {1, 0, 80, 40}, {1, 2, 110, 40}, {2, 2, 70, 40}, {1, 2, 140, 65}, {3, 4, 60, 30}});
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, edges, positions));
    CheckNoOverlap(bounds, positions);
    XII_TEST_BOOL(positions[1].x + bounds[1].width() < positions[2].x);
    XII_TEST_BOOL(positions[3].x + bounds[3].width() < positions[4].x);
    CheckRepeat(bounds, edges, positions);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hundreds of nodes with dense fan-out and long connections")
  {
    bounds.Clear();
    edges.Clear();
    for (xiiUInt32 i = 0; i < 240; ++i)
    {
      bounds.PushBack(QRectF(0, 0, 100 + (i % 4) * 70, 60 + (i % 7) * 25));
      if (i >= 12)
        for (xiiUInt32 offset = 0; offset < 5; ++offset)
          edges.PushBack({i - 12 + offset % 3, i, 30.0 + offset * 5, 30.0 + offset * 10});
      if (i >= 40)
        edges.PushBack({i - 40, i, 30, 30});
    }
    XII_TEST_RESULT(xiiNodeLayout::Compute(bounds, edges, positions));
    CheckNoOverlap(bounds, positions);
    for (const auto& edge : edges)
      XII_TEST_BOOL(positions[edge.m_uiSource].x + bounds[edge.m_uiSource].width() < positions[edge.m_uiTarget].x);
    CheckRepeat(bounds, edges, positions);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty and malformed inputs")
  {
    XII_TEST_RESULT(xiiNodeLayout::Compute({}, {}, positions));
    XII_TEST_BOOL(positions.IsEmpty());
    Assign(bounds, {QRectF(0, 0, 100, 50)});
    Assign(edges, {{0, 1, 0, 0}});
    XII_TEST_BOOL(xiiNodeLayout::Compute(bounds, edges, positions).Failed());
    XII_TEST_BOOL(positions.IsEmpty());
    bounds[0].setWidth(-1);
    XII_TEST_BOOL(xiiNodeLayout::Compute(bounds, {}, positions).Failed());
    XII_TEST_BOOL(positions.IsEmpty());
  }
}
