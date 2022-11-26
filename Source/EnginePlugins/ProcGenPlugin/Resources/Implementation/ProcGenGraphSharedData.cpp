#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace xiiProcGenInternal
{

  xiiUInt32 GraphSharedData::AddTagSet(const xiiTagSet& tagSet)
  {
    xiiUInt32 uiIndex = m_TagSets.IndexOf(tagSet);
    if (uiIndex == xiiInvalidIndex)
    {
      uiIndex = m_TagSets.GetCount();
      m_TagSets.PushBack(tagSet);
    }
    return uiIndex;
  }

  const xiiTagSet& GraphSharedData::GetTagSet(xiiUInt32 uiIndex) const { return m_TagSets[uiIndex]; }

  static xiiTypeVersion s_GraphSharedDataVersion = 1;

  void GraphSharedData::Save(xiiStreamWriter& stream) const
  {
    stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const xiiUInt32 uiCount = m_TagSets.GetCount();
      stream << uiCount;

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(stream);
      }
    }
  }

  xiiResult GraphSharedData::Load(xiiStreamReader& stream)
  {
    auto version = stream.ReadVersion(s_GraphSharedDataVersion);

    {
      xiiUInt32 uiCount = 0;
      stream >> uiCount;

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(stream, xiiTagRegistry::GetGlobalRegistry());
      }
    }

    return XII_SUCCESS;
  }

} // namespace xiiProcGenInternal
