/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Header file

#ifndef _DYNAMIC_RTSP_SERVER_HH
#define _DYNAMIC_RTSP_SERVER_HH

#ifndef _RTSP_SERVER_SUPPORTING_HTTP_STREAMING_HH
#include "RTSPServerSupportingHTTPStreaming.hh"
#endif

#include "CpuUtil.h"

class DynamicRTSPServer: public RTSPServerSupportingHTTPStreaming {
public:
  static DynamicRTSPServer* createNew(UsageEnvironment& env, Port ourPort,
				      UserAuthenticationDatabase* authDatabase,
				      unsigned reclamationTestSeconds = 65);

protected:
  DynamicRTSPServer(UsageEnvironment& env, int ourSocket, Port ourPort,
		    UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds);
  // called only by createNew();
  virtual ~DynamicRTSPServer();

protected: // redefined virtual functions
  virtual ServerMediaSession*
  lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession);

  virtual RTSPClientSession* createNewClientSession(u_int32_t sessionId)
  {
    return new LiveRTSPClientSession(*this, sessionId);
  }

  class LiveRTSPClientSession : public RTSPServer::RTSPClientSession 
  {
  public:
    LiveRTSPClientSession(DynamicRTSPServer& ourServer, unsigned sessionId)
      :RTSPClientSession(ourServer, sessionId),
      m_pParent(&ourServer)
    {

    }

  protected:
    virtual void handleCmd_SETUP(RTSPServer::RTSPClientConnection* ourClientConnection,
      char const* urlPreSuffix, char const* urlSuffix,
      char const* fullRequestStr)
    {
#ifndef _WIN32
      const double MAX_CPU_USAGE = 75.0;
      float dCpuUsage = m_pParent->cpuMonitor.GetCurrentProcCPULoad();
      fprintf( stderr, "Creating new RTSP client session: CPU load: %f\n", dCpuUsage);
      if (dCpuUsage >= MAX_CPU_USAGE)
      {
        fprintf(stderr, "Max CPU usage reached. Rejecting request\n");
        setRTSPResponse(ourClientConnection, "453 No Resources");
        return;
      }
#endif


      // Otherwise, handle the request as usual:
      RTSPClientSession::handleCmd_SETUP(ourClientConnection, urlPreSuffix, urlSuffix, fullRequestStr);
    }

    DynamicRTSPServer* m_pParent;
  };

  TaskToken m_logPerformanceTask;

  static void logCpuAndSessionInfoTask(DynamicRTSPServer* pRtspServer);
  void doLogCpuAndSessionInfoTask();

#ifndef _WIN32
  CpuMonitor cpuMonitor;
#endif
};

#endif

