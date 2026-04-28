/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if XII_DISABLED(XII_COMPILE_FOR_DEVELOPMENT)

inline void xiiLog::Dev(xiiLogInterface* /*pInterface*/, const xiiFormatString& /*string*/) {}

#endif

#if XII_DISABLED(XII_COMPILE_FOR_DEBUG)

inline void xiiLog::Debug(xiiLogInterface* /*pInterface*/, const xiiFormatString& /*string*/)
{}

#endif
