/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiDebugRendererLine::xiiDebugRendererLine() = default;

XII_ALWAYS_INLINE xiiDebugRendererLine::xiiDebugRendererLine(const xiiVec3& vStart, const xiiVec3& vEnd) :
  m_vStart(vStart), m_vEnd(vEnd)
{
}

XII_ALWAYS_INLINE xiiDebugRendererLine::xiiDebugRendererLine(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color) :
  m_vStart(vStart), m_vEnd(vEnd), m_StartColor(color), m_EndColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiDebugRendererTriangle::xiiDebugRendererTriangle() = default;

XII_ALWAYS_INLINE xiiDebugRendererTriangle::xiiDebugRendererTriangle(const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2)
{
  m_vPosition[0] = v0;
  m_vPosition[1] = v1;
  m_vPosition[2] = v2;
}
