#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class xiiGALScopedDebugGroup : public xiiReflectedClass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALScopedDebugGroup);

  XII_ADD_DYNAMIC_REFLECTION(xiiGALScopedDebugGroup, xiiReflectedClass);

public:
  xiiGALScopedDebugGroup() noexcept;
  xiiGALScopedDebugGroup(xiiGALCommandList* pCommandList, xiiStringView sName, xiiColor color = xiiColor::Black);
  ~xiiGALScopedDebugGroup();

private:
  xiiGALCommandList* m_pCommandList;
};

#include <GraphicsFoundation/Tools/Implementation/ScopedDebugGroup_inl.h>
