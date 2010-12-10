#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "profilersettings.h"
#include "CmdlineProfilerInterface.h"
#include <wx/process.h>
#include <wx/txtstrm.h>
#include "sampledata.h"
#include <wx/filename.h>


static  HANDLE s_hMapFile = INVALID_HANDLE_VALUE;
static char *s_pBuf = 0;
static wxString s_sharedMemFileName;



ProfilerProgressStatus *PrepareStatusInSharedMemory() {
  if (s_pBuf != 0) {
    CloseSharedMemory();
  }

  DWORD id = GetCurrentProcessId();
  char buf[256];
  sprintf(buf, "Global\\LukeStackWalkerStatus-%d", id);
  s_sharedMemFileName = buf;

   s_hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD) 
                 sizeof(ProfilerProgressStatus),   // maximum object size (low-order DWORD)  
                 buf);                 // name of mapping object
 
   if (s_hMapFile == NULL)  { 
      _tprintf(TEXT("Could not create file mapping object (%d).\n"), 
             GetLastError());
      return 0;
   }
   s_pBuf = (LPTSTR) MapViewOfFile(s_hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,                   
                        0,                   
                        sizeof(ProfilerProgressStatus));           
 
   if (s_pBuf == NULL)  { 
      _tprintf(TEXT("Could not map view of file (%d).\n"), 
             GetLastError()); 
	    CloseHandle(s_hMapFile);
      return 0;
   }
   memset(s_pBuf, 0, sizeof(ProfilerProgressStatus));
   return (ProfilerProgressStatus *)s_pBuf;
};

class MyPipedProcess : public wxProcess {
public:
    bool m_bRunning;
    MyPipedProcess() : wxProcess(0) {
       Redirect();
       m_bRunning = false;
    }

    virtual void OnTerminate(int /*pid*/, int /*status*/) {
      m_bRunning = false;
    }

    virtual bool HasInput() {
       bool hasInput = false;

      if ( IsInputAvailable() ) {
        wxTextInputStream tis(*GetInputStream());

        // this assumes that the output is always line buffered
        wxString msg;
        msg << tis.ReadLine();
        LogMessage(false, msg);
        hasInput = true;
      }
      if ( IsErrorAvailable() ) {
          wxTextInputStream tis(*GetErrorStream());
          // this assumes that the output is always line buffered
          wxString msg;
          msg << tis.ReadLine();
          LogMessage(true, msg);
          hasInput = true;
      }
      return hasInput;

    }
};


static MyPipedProcess s_process;
static wxString s_resultsName;
static wxString s_settingsName;


bool SampleWithCommandLineProfiler(ProfilerSettings *settings, unsigned int processId) {
  char exeFileName[MAX_PATH];
  GetModuleFileName(0, exeFileName, sizeof(exeFileName));
  wxFileName exeName(exeFileName);
  wxString exeDir = exeName.GetPath(wxPATH_GET_VOLUME|| wxPATH_GET_SEPARATOR);
  DWORD id = GetCurrentProcessId();
  char buf[2048];
  sprintf(buf, "%-tmp", id);
  // save settings to temp file
  wxString subdir = "cmdline-profiler\\";
  s_settingsName = exeDir + subdir + wxString(buf) + wxString(".lsp");
  s_resultsName = exeDir + subdir + wxString(buf) + wxString(".lsd");
#ifdef _DEBUG
  wxString cmdLineProfiler = exeDir + subdir + "cmdline-profiler_D.exe";
#else
  wxString cmdLineProfiler = exeDir + subdir + "cmdline-profiler.exe";
#endif
  _unlink(s_resultsName);
  _unlink(s_settingsName);
  Sleep(200);
  if (!settings->SaveAs(s_settingsName.c_str())) {
    LogMessage(true, "Failed to save settings file [%s] for command line profiler!", s_settingsName.c_str());
    return false;
  }

  sprintf(buf, "%s -in \"%s\" -out \"%s\" -shm %s -pid %d", cmdLineProfiler.c_str(), s_settingsName.c_str(), s_resultsName.c_str(), s_sharedMemFileName.c_str(), processId);
  long pid = wxExecute(buf, wxEXEC_ASYNC, &s_process);
  if (pid) {
    s_process.m_bRunning = true;
  } else {
    LogMessage(true, "Failed to execute command line profiler [%s]!", buf);
  }
  return !!pid;
}


bool HandleCommandLineProfilerOutput() {  
  while (s_process.HasInput()) {};      
  return s_process.m_bRunning;
}

void CloseSharedMemory() {
  UnmapViewOfFile(s_pBuf);
  CloseHandle(s_hMapFile);
  s_pBuf = 0;
}



bool FinishCmdLineProfiling() {
  Sleep(500);
  bool bRet = true;
  if (!LoadSampleData(s_resultsName)) {
   LogMessage(true, "Failed to load profile data from command line profiler!");
   bRet = false;
  } else {
    _unlink(s_resultsName);
  }
  _unlink(s_settingsName);
  return bRet;
}
