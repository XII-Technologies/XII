#pragma once

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Basics.h>
#  include <Foundation/Communication/Implementation/MessageLoop.h>

class XII_FOUNDATION_DLL xiiMessageLoop_mobile : public xiiMessageLoop
{
public:
  xiiMessageLoop_mobile();
  ~xiiMessageLoop_mobile();

protected:
  virtual void WakeUp() override;
  virtual bool WaitForMessages(xiiInt32 iTimeout, xiiIpcChannel* pFilter) override;

private:
};

#endif
