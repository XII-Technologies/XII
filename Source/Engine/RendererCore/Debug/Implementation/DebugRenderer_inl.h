
XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line() = default;

XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line(const xiiVec3& start, const xiiVec3& end) :
  m_start(start), m_end(end)
{
}

XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line(const xiiVec3& start, const xiiVec3& end, const xiiColor& color) :
  m_start(start), m_end(end), m_startColor(color), m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiDebugRenderer::Triangle::Triangle() = default;

XII_ALWAYS_INLINE xiiDebugRenderer::Triangle::Triangle(const xiiVec3& p0, const xiiVec3& p1, const xiiVec3& p2)

{
  m_position[0] = p0;
  m_position[1] = p1;
  m_position[2] = p2;
}
