#include "CpuUtil.h"
#ifdef _WIN32
#else
#include <fstream>
#include <sstream>
#include <syscall.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
#else
bool CpuUtil::ReadCpuInfo(CpuInfo& cpuInfo, std::map<std::string, CpuUtil::CpuInfo>& cpuCoreInfo )
{
    std::fstream inCpu("/proc/stat", std::ios_base::in | std::ios_base::binary);
    if (!inCpu.is_open())
      return false;
    // read the total first and then each core info
    std::string sCpu;
    unsigned long long uiUser =0, uiNice = 0, uiSystem = 0, uiIdle = 0, uiIoWait = 0, uiIrq = 0, uiSoftIrq = 0, uiSteal = 0;
    std::string sDump;
    inCpu >> sCpu >> uiUser >> uiNice >> uiSystem >> uiIdle >> uiIoWait >> uiIrq >> uiSoftIrq >> uiSteal >> sDump >> sDump;
    unsigned long long uiTotal = uiUser + uiNice + uiSystem + uiIdle + uiIoWait + uiIrq + uiSoftIrq + uiSteal;
    cpuInfo.idle = uiIdle + uiIoWait;
    cpuInfo.total = uiTotal;
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
    for (int count = 0; count < numCPU; ++ count)
    {
      std::string sCoreCpu;
      unsigned long long uiCoreUser =0, uiCoreNice = 0, uiCoreSystem = 0, uiCoreIdle = 0, uiCoreIoWait = 0, uiCoreIrq = 0, uiCoreSoftIrq = 0, uiCoreSteal = 0;
      std::string sCoreDump;
      inCpu >> sCoreCpu >> uiCoreUser >> uiCoreNice >> uiCoreSystem >> uiCoreIdle >> uiCoreIoWait >> uiCoreIrq >> uiCoreSoftIrq >> uiCoreSteal >> sCoreDump >> sCoreDump;
      unsigned long long uiCoreTotal = uiCoreUser + uiCoreNice + uiCoreSystem + uiCoreIdle + uiCoreIoWait + uiCoreIrq + uiCoreSoftIrq + uiCoreSteal;
      CpuInfo tempCpuInfo;
      tempCpuInfo.idle = uiCoreIdle + uiCoreIoWait;
      tempCpuInfo.total = uiCoreTotal;
      cpuCoreInfo[sCoreCpu] = tempCpuInfo;
    }
    inCpu.close();
    return true;
}

bool CpuUtil::ReadCpuUptimeInfo(CpuUptimeInfo &cpuUptimeInfo)
{

  std::fstream inUpTime("/proc/uptime", std::ios_base::in | std::ios_base::binary);
  if (!inUpTime.is_open())
    return false;
  unsigned long long uiTotalTime = 0, uiIdleTime = 0;
  inUpTime >> uiTotalTime >> uiIdleTime;
  inUpTime.close();

  cpuUptimeInfo.totalTime = uiTotalTime;
  cpuUptimeInfo.idleTime = uiIdleTime;
  //VLOG(2) << "Total Time: " << uiTotalTime << " Idle Time: " << uiIdleTime;

  return true;
}

bool CpuUtil::ReadThreadCPUInfo(const unsigned int uiProcId, const unsigned int uiThreadId, ThreadCpuInfo& threadCpuInfo)
{
    std::ostringstream ostrThreadFile("");
    ostrThreadFile << "/proc/" << uiProcId << "/task/" << uiThreadId  << "/stat";
    std::fstream threadIn( ostrThreadFile.str().c_str(), std::ios_base::in | std::ios_base::binary);
    if (!threadIn.is_open())
    {
      return false;
    }
    std::string sDump;
    unsigned long long uiThreadUser = 0 , uiThreadKernal = 0, uiCoreNumber = 0;
    // There are 13 unused field before the utime and stime
    for (int count = 0; count < 13; ++count)
    {
      threadIn >> sDump;
    }
    threadIn >> uiThreadUser >> uiThreadKernal;

    // Trying to get to position 39
    std::ostringstream ostrDump("");
    for (int count = 0; count < 23; ++count)
    {
        threadIn >> sDump;
    }
    threadIn >> uiCoreNumber;
    threadIn.close();
    threadCpuInfo.utime = uiThreadUser;
    threadCpuInfo.stime = uiThreadKernal;
    threadCpuInfo.coreNumber = uiCoreNumber;
    return true;
}

bool CpuUtil::ReadProcCpuInfo(const unsigned int uiProcId, ProcCpuInfo& procCpuInfo)
{
    std::ostringstream ostrProcFile("");
    ostrProcFile << "/proc/" << uiProcId << "/stat";
    std::fstream procIn( ostrProcFile.str().c_str(), std::ios_base::in | std::ios_base::binary);
    if (!procIn.is_open())
    {
      return false;
    }
    std::string sDump;
    unsigned long long uiProcUser = 0 , uiProcKernal = 0, uiProcChildUser = 0, uiProcChildKernal = 0;
    unsigned long long uiStartTime = 0;
    // There are 13 unused field before the utime and stime
    for (int count = 0; count < 13; ++count)
    {
      procIn >> sDump;
    }
    procIn >> uiProcUser >> uiProcKernal >> uiProcChildUser >> uiProcChildKernal;
    for (int count = 0; count < 4; ++count)
    {
      procIn >> sDump;
    }
    procIn >> uiStartTime;
    procIn.close();
    procCpuInfo.utime = uiProcUser;
    procCpuInfo.stime = uiProcKernal;
    procCpuInfo.cutime = uiProcChildUser;
    procCpuInfo.cstime = uiProcChildKernal;
    // This Field should never change
    procCpuInfo.startTime = uiStartTime;
    return true;
}

// This implements task id for linux (Should have been implemented in glibc)
pid_t CpuUtil::gettid(void)
{
    return syscall( __NR_gettid);
}

int CpuUtil::GetCurrentProcId()
{
    return getpid();
}
int CpuUtil::GetCurrentThreadId()
{
    return gettid();
}

int CpuUtil::GetSystemTick()
{
  return sysconf(_SC_CLK_TCK);
}

int CpuUtil::GetProcIdByName(std::string procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                std::getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }

    closedir(dp);

    return pid;
}

void CpuMonitor::manageCpuInfo()
{
  readCpuInfo();
  calculateCpuInfo();
  updateCpuInfo();
}
void CpuMonitor::addThreadIdToCpuWatchList(unsigned int threadId)
{
  if (!m_threadIdWatchSet.count(threadId))
  {
    m_threadIdWatchSet.insert(threadId);
  }
  else
  {
    //LOG(WARNING) << "Thread Id: " << threadId << " already exist in the watch list";
  }
}
void CpuMonitor::removeThreadIdFromCpuWatchLIst(unsigned int threadId)
{
  if(m_threadIdWatchSet.count(threadId))
  {
    m_threadIdWatchSet.erase(threadId);
  }
  else
  {
    //LOG(WARNING) << "Thread Id: " << threadId << " does not exist in the watch list";
  }
}

void CpuMonitor::readCpuInfo()
{
  // TODO: What happens when we fails to read
  // Read Current
  CpuUtil::ReadProcCpuInfo(CpuUtil::GetCurrentProcId(),m_procCpuInfo);
  CpuUtil::ReadCpuInfo(m_cpuInfo, m_cpuCoreInfo);
  CpuUtil::ReadCpuUptimeInfo(m_cpuUptimeInfo);
  for (std::set<unsigned>::iterator threadId = m_threadIdWatchSet.begin(); threadId != m_threadIdWatchSet.end(); ++threadId)
  {
    CpuUtil::ThreadCpuInfo threadCpuInfo;
    if (CpuUtil::ReadThreadCPUInfo(CpuUtil::GetCurrentProcId(), *threadId, threadCpuInfo))
    {
      m_threadCpuInfo[*threadId] = threadCpuInfo;
    }
    else
    {
      //LOG(WARNING) << "Failed to read thread cpu info on thread: " << threadId;
    }
  }
  // TODO: Add thread info
}
void CpuMonitor::updateCpuInfo()
{
  // Store the current to the previous value
  m_prevCpuInfo = m_cpuInfo;
  m_prevCpuCoreInfo = m_cpuCoreInfo;
  m_prevProcCpuInfo = m_procCpuInfo;
  m_prevCpuUptimeInfo = m_cpuUptimeInfo;
  m_prevCpuCoreInfo = m_cpuCoreInfo;
  m_prevThreadCpuInfo = m_threadCpuInfo;

  // For first time measurement
  if (!m_bInitialised)
  {
    m_bInitialised = true;
  }
}
float CpuMonitor::calculateCountTickToJiffieConversionFactor()
{
  return (m_cpuInfo.total - m_prevCpuInfo.total) /
    static_cast<float>((m_cpuUptimeInfo.totalTime - m_prevCpuUptimeInfo.totalTime) * CpuUtil::GetSystemTick());
}

void CpuMonitor::calculateCpuInfo()
{
  if (m_bInitialised)
  {
    float countTickToJiffie = calculateCountTickToJiffieConversionFactor();
    if (m_cpuInfo.total - m_prevCpuInfo.total == 0)
    {
      // no new info since last call
    }
    else
    {
      // should we mult by 100 to get percentage? TODO: check windows behaviour of this method
      // Calculate Cpu used by OS measured in jiffies
      m_fCpuUsage = (100.0*( (m_cpuInfo.total - m_prevCpuInfo.total) - (m_cpuInfo.idle - m_prevCpuInfo.idle) ) /
      static_cast<float>((m_cpuInfo.total - m_prevCpuInfo.total)) );

      // Calculate Core Cpu used by OS measured in jiffies
      for (CpuMap_t::iterator it = m_cpuCoreInfo.begin(); it != m_cpuCoreInfo.end(); ++it)
      {
        std::pair<std::string, CpuUtil::CpuInfo> cpuCoreInfo = *it;
        m_fCpuCoreUsage[cpuCoreInfo.first] = (100.0*( (cpuCoreInfo.second.total - m_prevCpuCoreInfo[cpuCoreInfo.first].total) - (cpuCoreInfo.second.idle - m_prevCpuCoreInfo[cpuCoreInfo.first].idle) ) /
        static_cast<float>(cpuCoreInfo.second.total - m_prevCpuCoreInfo[cpuCoreInfo.first].total) );
      }

      // Calculate Cpu used by process measured in clock ticks
      m_fProcCpuUsage = (100.0*(m_procCpuInfo.getUsage() - m_prevProcCpuInfo.getUsage()) * countTickToJiffie /
      static_cast<float>((m_cpuInfo.total - m_prevCpuInfo.total)) );

      // Calculate Cpu used by process and child process
      m_fProcCpuUsageWithChild = (100.0*(m_procCpuInfo.getUsageWithChild() - m_prevProcCpuInfo.getUsageWithChild()) /
      (static_cast<float>(CpuUtil::GetSystemTick())  *
      static_cast<float>((m_cpuUptimeInfo.totalTime - m_prevCpuUptimeInfo.totalTime))) );

      // Calculate Cpu used by watch list threads
      for (ThreadCpuInfoMap_t::iterator it =  m_threadCpuInfo.begin(); it != m_threadCpuInfo.end(); ++it)
      {
        std::pair<unsigned, CpuUtil::ThreadCpuInfo> threadCpuInfo = *it;
        float fThreadCpuUsage = (100.0 * (threadCpuInfo.second.getUsage() - m_prevThreadCpuInfo[threadCpuInfo.first].getUsage()) * countTickToJiffie /
          static_cast<float>((m_cpuInfo.total - m_prevCpuInfo.total)));
        unsigned int coreNumber = threadCpuInfo.second.coreNumber;
        m_threadCpuUsage[threadCpuInfo.first] = std::make_pair(fThreadCpuUsage, coreNumber);
      }
      // Calculate Cpu time interval
      m_fCpuTimeInterval = (m_cpuUptimeInfo.totalTime - m_prevCpuUptimeInfo.totalTime);
    }
    //std::ostringstream ostrCpuUsage("");
    //ostrCpuUsage << "Cpu System: " << m_fCpuUsage << "Process Id: " << CpuUtil::GetCurrentProcId()
    //             << " Cpu Process: " << m_fProcCpuUsage  << " Cpu Process with Child: " << m_fProcCpuUsageWithChild << std::endl;
    //VLOG(2) << ostrCpuUsage.str();
  }
}


#endif

