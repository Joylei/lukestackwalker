#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#if _MSC_VER >= 1300
#include <Tlhelp32.h>
#endif

#include "StackWalker.h"
#include "sampledata.h"
#include <wx/log.h>
#include <wx/filename.h>

std::map<unsigned int, ThreadSampleInfo> g_threadSamples;
int m_allThreadSamples = 0;

// Simple implementation of an additional output to the console:
class MyStackWalker : public StackWalker
{
public:
  FunctionSample *m_prevEntry;
  FunctionSample *m_firstEntry;
  Caller *m_prevCaller;
  bool m_bSkipFirstEntry;
  ThreadSampleInfo *m_currThreadContext;

  MyStackWalker(int options, DWORD dwProcessId, HANDLE hProcess, LPCSTR debugInfoPath) : StackWalker(options, debugInfoPath, dwProcessId, hProcess) {
    m_bSkipFirstEntry = false;
  }
  virtual void OnOutput(LPCSTR szText) { wxLogMessage(szText); }

  void OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry) {  
    if ( (eType == lastEntry) || (entry.offset == 0) ) {
      return;
    }
    char *name = "";
    char addrbuf[256];
    if (entry.name[0] == 0)
      name = entry.name;
    if (entry.undName[0] != 0)
      name = entry.undName;
    if (entry.undFullName[0] != 0)
      name = entry.undFullName;
    if (name[0] == 0) {
      if (entry.moduleName[0]) {
        sprintf(addrbuf, "%s : 0x%08X", entry.moduleName, (int)entry.offset);
      } else {
        sprintf(addrbuf, "0x%08X", (int)entry.offset);
      }
      name = addrbuf;
    }

    if (!strcmp(name, "KiFastSystemCallRet")) {
      m_bSkipFirstEntry = true;
      return;
    } 
    if (eType != firstEntry && m_bSkipFirstEntry) {
      eType = firstEntry;
      m_bSkipFirstEntry = false;
    } else if (eType == firstEntry) {
      m_bSkipFirstEntry = false;
    }


    if (name[0]) {
      std::map<std::string, FunctionSample>::iterator it = m_currThreadContext->m_functionSamples.find(name);
      if (it == m_currThreadContext->m_functionSamples.end()) {
        FunctionSample fs;
        fs.m_functionName = name;
        fs.m_maxLine = entry.lineNumber;
        fs.m_minLine = entry.lineNumber;
        m_currThreadContext->m_functionSamples[name] = fs;
        it = m_currThreadContext->m_functionSamples.find(name);
        Caller c;
        c.m_functionSample = &it->second;        
        it->second.m_callgraph.push_back(c);
      }

      if (it->second.m_fileName.empty() && entry.lineFileName[0]) {
        it->second.m_fileName = entry.lineFileName;
      }
      if (it->second.m_moduleName.empty() && entry.moduleName[0]) {
        it->second.m_moduleName = entry.moduleName;
      }
      if ((int)entry.lineNumber < it->second.m_minLine) {
        it->second.m_minLine = entry.lineNumber;
      }
      if ((int)entry.lineNumber > it->second.m_maxLine) {
        it->second.m_maxLine = entry.lineNumber;
      }      
      if ((eType == firstEntry)) {
        // top level entry handling
        it->second.m_sampleCount++;
        it->second.m_callgraph.front().m_sampleCount++;        
        m_firstEntry = &it->second;
        m_prevCaller = &it->second.m_callgraph.front();
      } else {
        // 2nd-nth level entry handling
        Caller *pc = 0;
        // find if this caller is already a node in the call graph of this top-of-stack function
        for (std::list<Caller>::iterator cit = m_firstEntry->m_callgraph.begin();
          cit != m_firstEntry->m_callgraph.end(); ++cit) {
            if ((cit->m_functionSample == &(it->second)) &&
              (cit->m_lineNumber == (int)entry.lineNumber)) {
                pc = &(*cit);
                break;
            }
        }
        if (!pc) { // if not, add it
          Caller c;
          c.m_functionSample = &(it->second);
          c.m_lineNumber = entry.lineNumber;
          m_firstEntry->m_callgraph.push_back(c);
          pc = &m_firstEntry->m_callgraph.back();
        }
        pc->m_sampleCount++;
        Callee *pCallee = 0;
        // then find if the edge already exists in the call graph
        std::list<Callee>::iterator calleeIt = pc->m_callees.begin();
        for (; calleeIt != pc->m_callees.end(); ++calleeIt) {
          if (calleeIt->m_target == m_prevCaller) {
            pCallee = &(*calleeIt);
            break;
          }
        }
        // if not, add it
        if (calleeIt == pc->m_callees.end()) {
          Callee callee;
          callee.m_target = m_prevCaller;
          pc->m_callees.push_back(callee);
          pCallee = &pc->m_callees.back();
        }
        pCallee->m_count++;
        m_prevCaller = pc;
      }
      m_prevEntry = &it->second;

    }   

    if (entry.lineFileName[0]) {
      if ((eType == firstEntry)) {              
        std::map<std::string, FileLineInfo>::iterator flit =  m_currThreadContext->m_lineSamples.find(entry.lineFileName);
        if (flit == m_currThreadContext->m_lineSamples.end()) {
          FileLineInfo fli;
          fli.m_fileName = name;        
          m_currThreadContext->m_lineSamples[entry.lineFileName] = fli;
          flit =  m_currThreadContext->m_lineSamples.find(entry.lineFileName);
          flit->second.m_lineSamples.reserve(2 * entry.lineNumber);
        }
        if (flit->second.m_lineSamples.size() <= entry.lineNumber) {
          flit->second.m_lineSamples.resize(entry.lineNumber+1);
        }
        flit->second.m_lineSamples[entry.lineNumber].m_sampleCount++;
      }      
    }

  }
};

std::list<FunctionSample *> g_sortedFunctionSamples;
bool FunctionSamplePredicate (const FunctionSample *lhs, const FunctionSample *rhs) {
  return lhs->m_sampleCount < rhs->m_sampleCount;
}

void SortFunctionSamples(ThreadSampleInfo *threadInfo) {
  if (!threadInfo->m_functionSamples.size())
    return;
  threadInfo->m_sortedFunctionSamples.clear();
  for (std::map<std::string, FunctionSample>::iterator it = threadInfo->m_functionSamples.begin();
    it != threadInfo->m_functionSamples.end(); ++it) {
      if (it->second.m_sampleCount) {
        threadInfo->m_sortedFunctionSamples.push_back(&it->second);
      }
  }
  threadInfo->m_sortedFunctionSamples.sort(FunctionSamplePredicate);
}


double ProfileProcess(DWORD dwProcessId, LPCSTR debugInfoPath, int maxDepth, time_t duration,  ProfilerProgressStatus *status, bool bConnectToServer) {
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);  

  HANDLE hProcess = OpenProcess(
    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    FALSE, dwProcessId);

  if (hProcess == INVALID_HANDLE_VALUE)
    return 0;


  // Initialize StackWalker...
  int options = MyStackWalker::OptionsAll;
  if (!bConnectToServer) {
    options &= ~MyStackWalker::SymUseSymSrv;
  }
  MyStackWalker sw(options, dwProcessId, hProcess, debugInfoPath);
  sw.LoadModules();


  // now enum all threads for this processId
  
  time_t end = 0;
  HANDLE hSnap = INVALID_HANDLE_VALUE;
  bool bExited = false;
  time_t samplestart = time(0);
  int nLoops = 0;
  do {
    if ((nLoops++ % 10 == 0) || (hSnap == INVALID_HANDLE_VALUE)) { // check for new threads once every 10 samples
      CloseHandle(hSnap);
      hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessId);
      if (hSnap == INVALID_HANDLE_VALUE) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
          if (exitCode != STILL_ACTIVE)  {
            wxLogMessage( "Target program exited with code 0x%08x.", exitCode);
          }
        } else {
          wxLogMessage( "CreateToolhelp32Snapshot failed with error code 0x%08x.", GetLastError());
        }
        CloseHandle(hProcess);
        return 0;
      }
    }

    THREADENTRY32 te;  
    memset(&te, 0, sizeof(te));
    te.dwSize = sizeof(te);
    if (Thread32First(hSnap, &te) == FALSE) {
      break;
    }   
    do {
      if (te.th32OwnerProcessID != dwProcessId)
        continue;
      HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
      if (hThread == NULL)
        continue;
      sw.m_currThreadContext = &g_threadSamples[te.th32ThreadID];
      sw.ShowCallstack(hThread, maxDepth);
      m_allThreadSamples++;
      sw.m_currThreadContext->m_totalSamples++;
      CloseHandle(hThread);
      DWORD exitCode = 0;
      if (GetExitCodeProcess(hProcess, &exitCode)) {
        if (exitCode != STILL_ACTIVE)  {
          wxLogMessage( "Target program exited with code 0x%08x.", exitCode);
          bExited = true;
        }
      } else {
        bExited = true;
      }
      if (bExited)
        break;
    } while(Thread32Next(hSnap, &te) != FALSE);    
    Sleep(1);
    if (!end) { // 1st sample loads symbols; set end time after that...
      end = time(0) + duration;
    }
    status->secondsLeftToProfile = end - time(0);
    if (status->bFinishedSampling)
      break;
    if (bExited)
      break;

  } while ((time(0) < end) || (duration == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL));
  time_t sampleend = time(0);
  double sampleSpeed = (double)m_allThreadSamples  / (sampleend - samplestart);

  if (hSnap != INVALID_HANDLE_VALUE)
    CloseHandle(hSnap);

  CloseHandle(hProcess);
  SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
  return sampleSpeed;
}

void SelectThreadForDisplay(unsigned int threadId, bool bSelect) {
  for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
       it != g_threadSamples.end(); it++) {
     if (it->first == threadId) {
       it->second.m_bSelectedForDisplay = bSelect;
     }
  }
}

void CopyCallees(Caller *src, Caller *dst, FunctionSample *dstFs) {
  for (std::list<Callee>::iterator srcit = src->m_callees.begin(); 
       srcit != src->m_callees.end(); ++srcit) {
    bool bFound = false;
    for (std::list<Callee>::iterator dstit = dst->m_callees.begin(); 
       dstit != dst->m_callees.end(); ++dstit) {
         if ((dstit->m_target->m_functionSample->m_functionName ==
              srcit->m_target->m_functionSample->m_functionName) &&
              (srcit->m_target->m_functionSample->m_fileName ==
               dstit->m_target->m_functionSample->m_fileName) &&
               (srcit->m_target->m_functionSample->m_moduleName ==
               dstit->m_target->m_functionSample->m_moduleName) &&
               (dstit->m_target->m_lineNumber == srcit->m_target->m_lineNumber)) {
            bFound = true;
            dstit->m_count += srcit->m_count;
            break;
         }
    }
    if (!bFound) {
      // search the right caller in the destination functionsample call graph
      for (std::list<Caller>::iterator cgit = dstFs->m_callgraph.begin();
           cgit != dstFs->m_callgraph.end(); ++cgit) {
             if ((cgit->m_functionSample->m_functionName == srcit->m_target->m_functionSample->m_functionName) &&
          (srcit->m_target->m_functionSample->m_fileName == cgit->m_functionSample->m_fileName) &&
          (srcit->m_target->m_functionSample->m_moduleName == cgit->m_functionSample->m_moduleName)) {
            Callee c;
            c.m_count = srcit->m_count;
            c.m_target = &(*cgit);
            dst->m_callees.push_back(c);
          }        
      }
    }
  }
}

ThreadSampleInfo *g_displayedSampleInfo = 0;
void ProduceDisplayData() {
  static ThreadSampleInfo summedSampleInfo;
  ThreadSampleInfo *onlySelectedThread = 0;
  int nSelected = 0;
  for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
       it != g_threadSamples.end(); it++) {
    if (it->second.m_bSelectedForDisplay) {
      nSelected++;
      onlySelectedThread = &it->second;      
    }
  }
  if (nSelected == 1) {
    g_displayedSampleInfo = onlySelectedThread;    
  } else {
    summedSampleInfo.m_totalSamples = 0;
    summedSampleInfo.m_functionSamples.clear();
    summedSampleInfo.m_lineSamples.clear();
    summedSampleInfo.m_sortedFunctionSamples.clear();
    for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
       it != g_threadSamples.end(); it++) {
       ThreadSampleInfo *tsi = &it->second;
       if (it->second.m_bSelectedForDisplay) {
         // sum function sample data
         summedSampleInfo.m_totalSamples += tsi->m_totalSamples;

         for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
              fsit != tsi->m_functionSamples.end(); ++fsit) {
            FunctionSample *dest = &summedSampleInfo.m_functionSamples[fsit->first];
            dest->m_functionName = fsit->second.m_functionName;
            dest->m_fileName = fsit->second.m_fileName;
            dest->m_moduleName = fsit->second.m_moduleName;
            dest->m_sampleCount += fsit->second.m_sampleCount;
            if (dest->m_maxLine < fsit->second.m_maxLine) {
              dest->m_maxLine = fsit->second.m_maxLine;
            }
            if (dest->m_minLine > fsit->second.m_minLine) {
              dest->m_minLine = fsit->second.m_minLine;
            }
         }


         // sum file/line sample counts
         for (std::map<std::string, FileLineInfo>::iterator flit = tsi->m_lineSamples.begin();
           flit != tsi->m_lineSamples.end(); ++flit) {
             std::map<std::string, FileLineInfo>::iterator dest = summedSampleInfo.m_lineSamples.find(flit->first);
             if (dest == summedSampleInfo.m_lineSamples.end()) {
               summedSampleInfo.m_lineSamples[flit->first] = flit->second;
             } else {               
               if (dest->second.m_lineSamples.size() < flit->second.m_lineSamples.size()) {
                 dest->second.m_lineSamples.resize(flit->second.m_lineSamples.size());
               }
               for (int i = 0; i < (int)flit->second.m_lineSamples.size(); i++) {
                 dest->second.m_lineSamples[i].m_sampleCount += flit->second.m_lineSamples[i].m_sampleCount;
               }
             }
         }

       }
    }

    { // 2nd pass - sum call graph Callers
      for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
        it != g_threadSamples.end(); it++) {
        ThreadSampleInfo *tsi = &it->second;
        if (it->second.m_bSelectedForDisplay) {
          for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
              fsit != tsi->m_functionSamples.end(); ++fsit) {
            FunctionSample *srcFs = &fsit->second;
            FunctionSample *dstFs = &summedSampleInfo.m_functionSamples[fsit->first]; // must be found since it was added in previous pass
            // 1st pass - add all callers
            for (std::list<Caller>::iterator scit = srcFs->m_callgraph.begin(); scit != srcFs->m_callgraph.end(); ++scit) {
              bool bFound = false;
              for (std::list<Caller>::iterator dcit = dstFs->m_callgraph.begin(); dcit != dstFs->m_callgraph.end(); ++dcit) {
                if (dcit->m_functionSample->m_functionName  == scit->m_functionSample->m_functionName && 
                    dcit->m_functionSample->m_moduleName == scit->m_functionSample->m_moduleName && 
                    dcit->m_lineNumber == scit->m_lineNumber) {
                  bFound = true;
                  // Sum Caller data add functionSampleData 
                  dcit->m_sampleCount += scit->m_sampleCount;
                  break;
                }
              }
              if (!bFound) {                  
                Caller c;
                c.m_lineNumber = scit->m_lineNumber;
                c.m_sampleCount = scit->m_sampleCount;
                FunctionSample *callerFs = &summedSampleInfo.m_functionSamples[scit->m_functionSample->m_functionName]; 
                c.m_functionSample = callerFs;
                dstFs->m_callgraph.push_back(c);
              }
            }                  
         }
        }
      }
    }

    { // 3rd pass - sum callee lists
      for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
        it != g_threadSamples.end(); it++) {
        ThreadSampleInfo *tsi = &it->second;
        if (it->second.m_bSelectedForDisplay) {
          for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
              fsit != tsi->m_functionSamples.end(); ++fsit) {
            FunctionSample *srcFs = &fsit->second;
            FunctionSample *dstFs = &summedSampleInfo.m_functionSamples[fsit->first]; // must be found since it was added in previous pass
            // 1st pass - add all callers
            for (std::list<Caller>::iterator scit = srcFs->m_callgraph.begin(); scit != srcFs->m_callgraph.end(); ++scit) {
              for (std::list<Caller>::iterator dcit = dstFs->m_callgraph.begin(); dcit != dstFs->m_callgraph.end(); ++dcit) {
                if (dcit->m_functionSample->m_functionName  == scit->m_functionSample->m_functionName && 
                    dcit->m_functionSample->m_moduleName == scit->m_functionSample->m_moduleName && 
                    dcit->m_lineNumber == scit->m_lineNumber) {
                  // Sum Caller data add functionSampleData 
                  CopyCallees(&(*scit), &(*dcit), dstFs);
                  break;
                }
              }
            }                  
         }
        }
      }
    }



    g_displayedSampleInfo = &summedSampleInfo;

  }
  SortFunctionSamples(g_displayedSampleInfo);
}




PROCESS_INFORMATION LaunchTarget(const char *exe, const char *cmdline, const char *directory, char *env) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );


  wxFileName fn(exe);

  char realCommandLine[10240];
  sprintf(realCommandLine, "%s.%s %s", fn.GetName().c_str(), fn.GetExt().c_str(), cmdline);


  // Start the child process. 
  if( !CreateProcess( exe,   
    realCommandLine, // Command line
    NULL,           // Process handle not inheritable
    NULL,           // Thread handle not inheritable
    FALSE,          // Set handle inheritance to FALSE
    0,              // No creation flags
    env,           
    directory,          
    &si,            // Pointer to STARTUPINFO structure
    &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
  {
    char errBuffer[2048];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, errBuffer, sizeof(errBuffer), 0);
    while (strlen(errBuffer) && (errBuffer[strlen(errBuffer) - 1] == '\n' || errBuffer[strlen(errBuffer) - 1] == '\r')) {
      errBuffer[strlen(errBuffer) - 1] = 0;
    }
    wxLogMessage( "CreateProcess failed %d: [%s].", GetLastError(), errBuffer );
    return pi;
  }

  return pi;


}

char *MergeEnvironment(ProfilerSettings *settings) {
  char *env = GetEnvironmentStrings();
  char *envOrg = env;
  std::map<wxString, wxString> newEnv;
  while (env[0]) {
    wxString e = env;
    wxString var = e.BeforeFirst('=').MakeUpper();    
    wxString val = e.AfterFirst('=');
    newEnv[var] = val;
    env += strlen(env) + 1;
  }
  FreeEnvironmentStrings(envOrg);
  for (std::map<wxString, wxString>::iterator it = settings->m_environmentVariables.begin();
       it != settings->m_environmentVariables.end(); ++it) {
    wxString var = it->first;
    var = var.MakeUpper();
    newEnv[var] = it->second;
  }
  wxString newEnvString;
  for (std::map<wxString, wxString>::iterator it = newEnv.begin();
       it != newEnv.end(); ++it) {
    wxString e = it->first + wxString("=") + it->second;
    newEnvString += e + wxString("\n");
    //wxLogMessage(e);
  }
  newEnvString += wxString("\n");
  //wxLogMessage("\n");

  char *ret = new char[newEnvString.Len()+1];
  strcpy(ret, newEnvString.c_str());
  for (int i = 0; i < (int)newEnvString.Len(); i++) {
    if (ret[i] == '\n')
      ret[i] = 0;
  }
  return ret;
}

bool SampleProcess(ProfilerSettings *settings, ProfilerProgressStatus *status) {  

  g_threadSamples.clear();
  m_allThreadSamples = 0;

  wxLogMessage("Launching executable %s.", settings->m_executable.c_str());

  char *env = MergeEnvironment(settings);

  status->secondsLeftToStart = settings->m_samplingStartDelay;
  PROCESS_INFORMATION pi = LaunchTarget(settings->m_executable.c_str(),
    settings->m_commandLineArgs.c_str(), settings->m_currentDirectory.c_str(), env);

  delete [] env;

  if (!pi.dwProcessId) {
    return false;
  }
  timeBeginPeriod(1);
  WaitForInputIdle(pi.hProcess, 500);
  std::string debugPaths;
  for (std::list<wxString>::iterator it = settings->m_debugInfoPaths.begin();
    it != settings->m_debugInfoPaths.end(); ++it) {
      debugPaths += it->c_str();
      debugPaths += ";";
  }

  time_t end = time(0) + settings->m_samplingStartDelay;
  while ((time(0) < end) || 
    (settings->m_samplingStartDelay == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL)) {
      status->secondsLeftToStart = end - time(0);
      Sleep(100);
      if (status->bStartedSampling)
        break;
  }


  status->secondsLeftToProfile = settings->m_samplingTime;
  status->bStartedSampling = true;
  status->secondsLeftToStart = 0;
  double sampleSpeed = 0;
  if (!status->bFinishedSampling) {
    sampleSpeed = ProfileProcess(pi.dwProcessId, debugPaths.c_str(), settings->m_sampleDepth, settings->m_samplingTime, status, settings->m_bConnectToSymServer);
  }
  TerminateProcess(pi.hProcess, 0);
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
  wxLogMessage("Sorting profile data.");
  for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
       it != g_threadSamples.end(); it++) {     
    SortFunctionSamples(&it->second);
  }

  if (g_threadSamples.size()) 
    g_threadSamples.begin()->second.m_bSelectedForDisplay = true;
  ProduceDisplayData();
  wxLogMessage("Done; %d samples collected at %0.1lf samples/second.", m_allThreadSamples, sampleSpeed);
  status->bFinishedSampling = true;
  return true;
}


#include <wx/wfstream.h>
#include <wx/txtstrm.h>
enum {LSD_FILEVERSION = 1};
bool LoadSampleData(const wxString &fn) {

  wxFileInputStream file_input(fn);  
  wxTextInputStream in(file_input);
  
  g_threadSamples.clear();

  if (!file_input.IsOk())
    return false;

  if (in.ReadWord() != "Luke_Stackwalker_Data_(LSD)_file_version") {
    return false;
  }
  int fileVersion;
  in >> fileVersion;
  if (in.ReadWord() != "Threads") {
    return false;
  }
  int nThreadSamples = 0;
  in >> nThreadSamples;

  
  for (int i = 0; i < nThreadSamples; i++) {
    if (in.ReadWord() != "Thread_ID") {
      return false;
    }

    int threadId = 0;
    in >>  threadId;
    ThreadSampleInfo *tsi = &g_threadSamples[threadId];
    
    if (in.ReadWord() != "Total_Samples") {
      return false;
    }

    in >> tsi->m_totalSamples;

    if (in.ReadWord() != "Function_samples") {
      return false;
    }

    int nFs = 0;
    in >> nFs;
    
    for (int fsi = 0; fsi < nFs; fsi++) {      
      FunctionSample fs;
      fs.m_functionName = in.ReadLine();
      tsi->m_functionSamples[fs.m_functionName] = fs;
      FunctionSample &fs2 = tsi->m_functionSamples[fs.m_functionName];
      
      fs2.m_fileName = in.ReadLine();
      fs2.m_moduleName = in.ReadLine();
      in >> fs2.m_sampleCount;
      in >> fs2.m_maxLine;
      in >> fs2.m_minLine;      
    }
    for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
         fsit != tsi->m_functionSamples.end(); ++fsit) {
      if (in.ReadWord() != "Callers") {
       return false;
      }
      int nCallers = 0;
      in >> nCallers;
      for (int nc = 0; nc < nCallers; ++nc) {
        Caller c;
        std::string fn = in.ReadLine();
        c.m_functionSample = &tsi->m_functionSamples[fn];
        in >> c.m_sampleCount;
        in >> c.m_lineNumber;
        c.m_ordinalForSaving = nc;
        fsit->second.m_callgraph.push_back(c);
      }
    }

    for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
        fsit != tsi->m_functionSamples.end(); ++fsit) {      
      for (std::list<Caller>::iterator cit = fsit->second.m_callgraph.begin(); cit != fsit->second.m_callgraph.end(); ++cit) {
        if (in.ReadWord() != "Callees") {
          return false;
        }
        int nCallees = 0;
        in >> nCallees;
        for (int c = 0; c < nCallees; ++c) {
          Callee callee;
          in >> callee.m_count;
          int ordinal = 0;
          in >> ordinal;
          bool bFound = false;
          for (std::list<Caller>::iterator cit2 = fsit->second.m_callgraph.begin(); cit2 != fsit->second.m_callgraph.end(); ++cit2) {
            if (ordinal == cit2->m_ordinalForSaving) {
              callee.m_target = &(*cit2);
              bFound = true;
              break;
            }
          }
          if (!bFound)
            return false;
          cit->m_callees.push_back(callee);
        }        
      }
    }

    if (in.ReadWord() != "Files") {
      return false;
    }
    int nFiles = 0;
    in >> nFiles;

    // file/line sample counts
    for (int nFile = 0; nFile < nFiles; nFile++) {
      std::string name = in.ReadLine();
      FileLineInfo &fli = tsi->m_lineSamples[name]; 
      fli.m_fileName = name;
      if (in.ReadWord() != "Lines") {
        return false;
      }
      int nLines = 0;
      in >> nLines;
      fli.m_lineSamples.resize(nLines);
      for (int line = 0; line < nLines; line++) {
        in >> fli.m_lineSamples[line].m_sampleCount;        
      }
      std::string endOfLine = in.ReadLine();
    }
  }
  
  if (g_threadSamples.size()) 
    g_threadSamples.begin()->second.m_bSelectedForDisplay = true;
  ProduceDisplayData();
  return true;
}

bool SaveSampleData(const wxString &fn) {
  wxFileOutputStream file_output(fn);  
  wxTextOutputStream out(file_output);
  out << "Luke_Stackwalker_Data_(LSD)_file_version ";
  out << LSD_FILEVERSION << endl;
  out << "Threads ";
  out << g_threadSamples.size() << endl;

  for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
       it != g_threadSamples.end(); it++) {
    out << "Thread_ID  ";
    out << it->first << endl;
    ThreadSampleInfo *tsi = &it->second;

    out << "Total_Samples ";
    out << tsi->m_totalSamples << endl;

    out << "Function_samples ";
    out << tsi->m_functionSamples.size() << endl;

    for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
         fsit != tsi->m_functionSamples.end(); ++fsit) {
      out << fsit->second.m_functionName << endl;
      out << fsit->second.m_fileName << endl;
      out << fsit->second.m_moduleName << endl;
      out << fsit->second.m_sampleCount << endl;
      out << fsit->second.m_maxLine << endl;
      out << fsit->second.m_minLine << endl;      
    }
    for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
         fsit != tsi->m_functionSamples.end(); ++fsit) {
      out << "Callers " << fsit->second.m_callgraph.size() << endl;
      int ordinal = 0;
      for (std::list<Caller>::iterator cit = fsit->second.m_callgraph.begin(); cit != fsit->second.m_callgraph.end(); ++cit) {
        out << cit->m_functionSample->m_functionName << endl;
        out << cit->m_sampleCount << endl;
        out << cit->m_lineNumber << endl;
        cit->m_ordinalForSaving = ordinal;
        ordinal++;
      }
    }

    for (std::map<std::string, FunctionSample>::iterator fsit = tsi->m_functionSamples.begin();
         fsit != tsi->m_functionSamples.end(); ++fsit) {      
      for (std::list<Caller>::iterator cit = fsit->second.m_callgraph.begin(); cit != fsit->second.m_callgraph.end(); ++cit) {
        out << "Callees " << cit->m_callees.size() << endl;
        for (std::list<Callee>::iterator clit = cit->m_callees.begin(); clit != cit->m_callees.end(); ++clit) {
          out << clit->m_count << endl;
          out << clit->m_target->m_ordinalForSaving << endl;
        }        
      }
    }

    out << "Files ";
    out << tsi->m_lineSamples.size() << endl;
    
    for (std::map<std::string, FileLineInfo>::iterator flit = tsi->m_lineSamples.begin();
         flit != tsi->m_lineSamples.end(); ++flit) {
      out << flit->first << endl;
      out << "Lines ";
      out << flit->second.m_lineSamples.size() << endl;             
      for (int i = 0; i < (int)flit->second.m_lineSamples.size(); i++) {
        out << flit->second.m_lineSamples[i].m_sampleCount << " ";
      }
      out << endl;
    }
  }

  return file_output.GetLastError() == wxSTREAM_NO_ERROR;
}