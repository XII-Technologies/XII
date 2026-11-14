/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/NetworkResolver.h>
#include <Foundation/Network/TcpListener.h>
#include <Foundation/Network/UdpSocket.h>
#include <Foundation/Platform/Implementation/NetworkSocketAccess.h>
#include <Foundation/Strings/StringBuilder.h>

#include <climits>
#include <cmath>
#include <cstring>
#include <utility>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/Network_win.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Platform/Implementation/Posix/Network_posix.h>
#else
#  error "Native networking is not implemented on this platform."
#endif

#include <Foundation/Platform/Implementation/NetworkNative.h>
