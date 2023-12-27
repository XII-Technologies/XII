#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDragDropInfo, 1, xiiRTTIDefaultAllocator<xiiDragDropInfo>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDragDropInfo::xiiDragDropInfo()
{
  m_vDropPosition.Set(xiiMath::NaN<float>());
  m_vDropNormal.Set(xiiMath::NaN<float>());
  m_iTargetObjectSubID            = -1;
  m_iTargetObjectInsertChildIndex = -1;
  m_bShiftKeyDown                 = false;
  m_bCtrlKeyDown                  = false;
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDragDropConfig, 1, xiiRTTIDefaultAllocator<xiiDragDropConfig>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDragDropConfig::xiiDragDropConfig()
{
  m_bPickSelectedObjects = false;
}
