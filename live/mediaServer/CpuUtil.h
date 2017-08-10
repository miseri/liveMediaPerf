#pragma once
#ifdef _WIN32
#else
#include <set>
#include <map>
#include <memory>
#include <string>
#endif


// Linux Only for now
#ifdef _WIN32
#else

class CpuUtil
{

public:
  struct CpuInfo;
  struct CpuUptimeInfo;
  struct ProcCpuInfo;
  struct ThreadCpuInfo;

  static bool ReadCpuInfo(CpuInfo& cpuInfo, std::map<std::string, CpuUtil::CpuInfo>& cpuCoreInfo);
  static bool ReadCpuUptimeInfo(CpuUptimeInfo& cpuUptimeInfo);
  static bool ReadThreadCPUInfo(const unsigned int uiProcId, const unsigned int uiThreadId, ThreadCpuInfo& threadCpuInfo);
  static bool ReadProcCpuInfo(const unsigned int uiProcId, ProcCpuInfo& procCpuInfo);
  static int GetProcIdByName(std::string procName);

  static int GetCurrentProcId();
  static int GetCurrentThreadId();
  static int GetSystemTick();

private:
  // This implements task id for linux (Should have been implemented in glibc)
  static pid_t gettid(void);

};

struct CpuUtil::CpuInfo
{
  CpuInfo()
    :total(0), idle(0)
  {
  }
  unsigned long long total;
  unsigned long long idle;
};

struct CpuUtil::CpuUptimeInfo
{
  CpuUptimeInfo()
    :totalTime(0), idleTime(0)
  {
  }
  unsigned long long totalTime;
  unsigned long long idleTime;
};
struct CpuUtil::ProcCpuInfo
{
  ProcCpuInfo()
    :utime(0), stime(0.0f)
  {}
  unsigned long long getUsage()
  {
    return utime + stime;
  }
  unsigned long long getUsageWithChild()
  {
    return utime + stime + cutime + cstime;
  }

  unsigned long long utime;
  unsigned long long stime;
  unsigned long long cutime;
  unsigned long long cstime;
  unsigned long long startTime;
};

struct CpuUtil::ThreadCpuInfo
{
  ThreadCpuInfo()
    :utime(0), stime(0.0f), coreNumber(0)
  {}
  unsigned long long getUsage()
  {
    return utime + stime;
  }
  unsigned long long utime;
  unsigned long long stime;
  unsigned int coreNumber;

};

class CpuMonitor
{
public:
  CpuMonitor(unsigned int uiCheckIntervalS = 5) :
    m_uiIntervalS(uiCheckIntervalS),
    m_bInitialised(false),
    m_fCpuUsage(0.0),
    m_fProcCpuUsage(0.0),
    m_fProcCpuUsageWithChild(0.0),
    m_fCpuTimeInterval(0.0)
  {
  }

  /**
   * @brief returns CPU load as a percentage value.
   *
   * @return returns CPU load as a value between 0 and 100%.
   */
  float GetCurrentCPULoad()
  {
    return m_fCpuUsage;
  }
  std::map<std::string,float> GetCurrentCPUCoreLoad()
  {
    return m_fCpuCoreUsage;
  }
  float GetCurrentProcCPULoad()
  {
    return m_fProcCpuUsage;
  }
  float GetCurrentProcCPULoadWithChild()
  {
    return m_fProcCpuUsageWithChild;
  }
  float GetCurrentCPUTimeInterval()
  {
    return m_fCpuTimeInterval;
  }
  std::map<unsigned int, std::pair<float, unsigned int> > GetCurrentThreadCPULoadAndCore()
  {
    return m_threadCpuUsage;
  }

  void manageCpuInfo();
  void addThreadIdToCpuWatchList(unsigned int threadId);
  void removeThreadIdFromCpuWatchLIst(unsigned int threadId);

private:
  void readCpuInfo();
  void updateCpuInfo();
  void calculateCpuInfo();
  float calculateCountTickToJiffieConversionFactor();

private:

  unsigned m_uiIntervalS;

  bool m_bInitialised;
  // Store watch list of thread ids
  std::set<unsigned int> m_threadIdWatchSet;
  // Stores current
  CpuUtil::CpuInfo m_cpuInfo;
  CpuUtil::ProcCpuInfo m_procCpuInfo;
  typedef std::map<unsigned int, CpuUtil::ThreadCpuInfo> ThreadCpuInfoMap_t;
  ThreadCpuInfoMap_t m_threadCpuInfo;
  CpuUtil::CpuUptimeInfo m_cpuUptimeInfo;
  typedef std::map<std::string, CpuUtil::CpuInfo> CpuMap_t;
  CpuMap_t m_cpuCoreInfo;
  // Stores previous
  CpuUtil::CpuInfo m_prevCpuInfo;
  CpuUtil::ProcCpuInfo m_prevProcCpuInfo;
  ThreadCpuInfoMap_t m_prevThreadCpuInfo;
  CpuUtil::CpuUptimeInfo m_prevCpuUptimeInfo;
  CpuMap_t m_prevCpuCoreInfo;

  float m_fCpuUsage;
  float m_fProcCpuUsage;
  typedef std::pair<float, unsigned> CpuUsagePair_t;
  typedef std::map<unsigned int, CpuUsagePair_t> ThreadCpuUsageMap_t;
  ThreadCpuUsageMap_t m_threadCpuUsage;
  float m_fProcCpuUsageWithChild;
  float m_fCpuTimeInterval;
  typedef std::map<std::string, float> CpuCoreUsageMap_t;
  CpuCoreUsageMap_t m_fCpuCoreUsage;
};

#endif

