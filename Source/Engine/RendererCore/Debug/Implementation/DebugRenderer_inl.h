
XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line() = default;

XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line(const xiiVec3& vStart, const xiiVec3& vEnd) :
  m_start(vStart), m_end(vEnd)
{
}

XII_ALWAYS_INLINE xiiDebugRenderer::Line::Line(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color) :
  m_start(vStart), m_end(vEnd), m_startColor(color), m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiDebugRenderer::Triangle::Triangle() = default;

XII_ALWAYS_INLINE xiiDebugRenderer::Triangle::Triangle(const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2)

{
  m_position[0] = v0;
  m_position[1] = v1;
  m_position[2] = v2;
}
