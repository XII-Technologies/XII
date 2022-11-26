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

void operator>>(QDataStream& stream, xiiDynamicArray<xiiDocumentObject*>& rhs)
{
  int iIndices = 0;
  stream >> iIndices;
  rhs.Clear();
  rhs.Reserve(static_cast<xiiUInt32>(iIndices));

  for (int i = 0; i < iIndices; ++i)
  {
    void* p = nullptr;

    uint len = sizeof(void*);
    stream.readRawData((char*)&p, len);

    xiiDocumentObject* pDocObject = (xiiDocumentObject*)p;

    rhs.PushBack(pDocObject);
  }
}
