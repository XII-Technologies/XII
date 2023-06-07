#pragma once

#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace xiiProcGenInternal
{

  class XII_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    xiiUInt32 AddTagSet(const xiiTagSet& tagSet);

    const xiiTagSet& GetTagSet(xiiUInt32 uiIndex) const;

    void      Save(xiiStreamWriter& ref_stream) const;
    xiiResult Load(xiiStreamReader& ref_stream);

  private:
    xiiDynamicArray<xiiTagSet> m_TagSets;
  };

} // namespace xiiProcGenInternal
