#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include "sampledata.h"
#include <wx/thread.h>
#include "profileprogress.h"
#include <wx/timer.h>
#include <wx/log.h>
#include <wx/textCtrl.h>
#include <wx/stattext.h>
#include "CmdlineProfilerInterface.h"
#include <windows.h>

class ProfileProgressDialog : public wxDialog, public wxThread {
public:
    wxGauge *m_gauge;
    wxButton *m_cancelButton;
    wxButton *m_actionButton;    
    ProfileProgressDialog(wxWindow *parent, ProfilerSettings *settings);
    ProfilerSettings *m_settings;
    ProfilerProgressStatus m_statusData;
    ProfilerProgressStatus *m_status;
    bool m_bProfileReturnValue;
    bool m_bTitleChanged;
    bool m_bTitleChangedToLoading;
    bool m_bRunningOn64BitOS;
    bool m_bProfileUsingCmdLineProfiler;
    wxTimer m_timer;
    wxStaticText *m_staticText;
    unsigned int m_processId;

    void OnCancelButton(wxCommandEvent& event);
    void OnActionButton(wxCommandEvent& event);
    void OnPauseContinueButton(wxCommandEvent& event);

    void OnClose(wxCloseEvent& event);
    virtual ExitCode Entry();
    void OnTimer(wxTimerEvent& evt);
    void EndProfiling();

private:
    DECLARE_EVENT_TABLE()
};

enum {
  ID_CANCELBUTTON = wxID_HIGHEST + 1,  
  ID_ACTIONBUTTON
};

BEGIN_EVENT_TABLE(ProfileProgressDialog, wxDialog)
    EVT_BUTTON(ID_CANCELBUTTON, ProfileProgressDialog::OnCancelButton)
    EVT_BUTTON(ID_ACTIONBUTTON, ProfileProgressDialog::OnActionButton)    
    EVT_CLOSE(ProfileProgressDialog::OnClose)
    EVT_TIMER(wxID_ANY, ProfileProgressDialog::OnTimer)
END_EVENT_TABLE()


ProfileProgressDialog::ProfileProgressDialog(wxWindow *parent, ProfilerSettings *settings)
                : wxDialog(parent, wxID_ANY, wxString(_T("Waiting to start..."))),
                  wxThread(wxTHREAD_JOINABLE ),
                  m_timer(this) {
    m_bProfileUsingCmdLineProfiler = false;
    m_status = 0;
    m_settings = settings;
    wxBoxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);

    m_gauge = new wxGauge(this, wxID_ANY, 150, wxDefaultPosition, wxSize(150, 20), wxGA_HORIZONTAL | wxGA_SMOOTH );
    m_gauge->SetValue(100);
    m_gauge->SetRange(100);

    sizerTop->Add(m_gauge, 1, wxEXPAND | wxALL, 7);

    m_staticText = new wxStaticText(this, wxID_ANY, "999 seconds left, 99999 samples collected", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
    sizerTop->Add(m_staticText, 1, wxEXPAND | wxALL, 7);

    wxBoxSizer *sizerBot = new wxBoxSizer(wxHORIZONTAL);

    sizerTop->Add(sizerBot, 1, wxEXPAND, 5);

    m_actionButton = new wxButton(this, ID_ACTIONBUTTON, _T("Start Sampling"));
    sizerBot->Add(m_actionButton, 1, wxEXPAND | wxALL, 5);   
    
    m_cancelButton = new wxButton(this, ID_CANCELBUTTON, _T("Cancel"));
    sizerBot->Add(m_cancelButton, 1, wxEXPAND | wxALL, 5);

    
    SetSizer(sizerTop);

    sizerTop->SetSizeHints(this);
    sizerTop->Fit(this);
    CenterOnParent(wxBOTH);
    m_bTitleChanged = false;
    m_bTitleChangedToLoading = false;
    BOOL b64bitOS = false;
    IsWow64Process(GetCurrentProcess(), &b64bitOS);
    m_bRunningOn64BitOS = !!b64bitOS;
}

void ProfileProgressDialog::OnActionButton(wxCommandEvent& WXUNUSED(event)) {
  if (!m_status->bStartedSampling) {
    m_status->bStartedSampling = true;
    return;
  }
  if (!m_status->bFinishedSampling) {
    m_status->bFinishedSampling = true;
    return;
  }
}

void ProfileProgressDialog::OnCancelButton(wxCommandEvent& event) {
  if (m_bTitleChanged) {
    OnPauseContinueButton(event);
    return;
  }
  m_status->bFinishedSampling = true;
  m_status->bStartedSampling = true;
  EndProfiling();
}

void ProfileProgressDialog::OnPauseContinueButton(wxCommandEvent& WXUNUSED(event)) {
  m_status->bSamplingPaused = !m_status->bSamplingPaused;
  if (m_status->bSamplingPaused) {
    m_cancelButton->SetLabel("Continue");
    SetLabel("Paused");
  } else {
    m_cancelButton->SetLabel("Pause");
    SetLabel("Sampling...");
  }
}


void ProfileProgressDialog::EndProfiling() {
  if (m_bProfileUsingCmdLineProfiler) {
    m_bProfileReturnValue = FinishCmdLineProfiling();
    CloseSharedMemory();
  }
  if (IsRunning()) {
    Wait();
  }
  m_timer.Stop();
  EndModal(0);
}

void ProfileProgressDialog::OnClose(wxCloseEvent&) { 
  m_status->bFinishedSampling = true;
  m_status->bStartedSampling = true;
  EndProfiling(); 
}

wxThread::ExitCode ProfileProgressDialog::Entry() {  
  m_status = &m_statusData;
  m_bProfileReturnValue = SampleProcess(m_settings, m_status, m_processId);
  
  m_status->bFinishedSampling = true;
  return 0;
}

void ProfileProgressDialog::OnTimer(wxTimerEvent& WXUNUSED(evt)) {  
  if (m_bProfileUsingCmdLineProfiler) {
    if (!HandleCommandLineProfilerOutput()) {
      m_status->bFinishedSampling = true;
    }
  }
  if (!m_status->bStartedSampling) {
    if (m_settings->m_samplingStartDelay == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL) {
      m_staticText->SetLabel("Press 'start' to begin.");
      m_gauge->Pulse();
    } else {
      int val = m_status->secondsLeftToStart;
      m_gauge->SetRange(100 * m_settings->m_samplingStartDelay);
      m_gauge->SetValue(100 * val);
      char buf[256];
      sprintf(buf, "%d seconds to start", m_status->secondsLeftToStart);
      m_staticText->SetLabel(buf);
    }
  } else {
    if (!m_status->nSamplesTaken) {
      if (!m_bTitleChangedToLoading) {
        m_bTitleChangedToLoading = true;
        SetLabel("Loading debug info...");
        m_actionButton->Enable(false);
      }
      char buf[256];
      sprintf(buf, "%d of %d modules loaded", m_status->nLoadedModules, m_status->nTotalModules);
      m_staticText->SetLabel(buf);
      if (!m_status->nTotalModules) {        
        m_gauge->SetValue(0);
      } else {        
        m_gauge->SetRange(100 * m_status->nTotalModules);
        m_gauge->SetValue(100 * m_status->nLoadedModules);
      }
    } else {
      if (!m_bTitleChanged) {        
        m_actionButton->Enable(true);
        m_cancelButton->SetLabel("Pause");
        SetLabel("Sampling...");
        m_actionButton->SetLabel("Stop Sampling");
        m_bTitleChanged = true;
      }
      if (m_settings->m_samplingTime == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL) {
        char buf[256];
        sprintf(buf, "%d samples collected", m_status->nSamplesTaken);
        m_staticText->SetLabel(buf);
        if (!m_status->bSamplingPaused)
          m_gauge->Pulse();   
      } else {
        int val = m_settings->m_samplingTime - m_status->secondsLeftToProfile;
        m_gauge->SetRange(100 * m_settings->m_samplingTime);
        m_gauge->SetValue(100 * val);
        char buf[256];
        sprintf(buf, "%d seconds left, %d samples collected", m_status->secondsLeftToProfile, m_status->nSamplesTaken);
        m_staticText->SetLabel(buf);
      }
    }
    
  }
  if (m_status->bFinishedSampling) {
    EndProfiling();
  }
}


#include "ProcessEnumDialog.h"


bool SampleProcessWithDialogProgress(wxWindow *appMainWindow, ProfilerSettings *settings) {

  unsigned int processid = 0;
  if (settings->m_bAttachToProcess) {
    ProcessEnumDialog dlg(appMainWindow);
    int ret = dlg.ShowModal();
    if (ret == wxID_CANCEL) {
      return true;
    }
    processid = dlg.m_processId;
  }  

  ProfileProgressDialog dlg(appMainWindow, settings); 
  dlg.m_processId = processid; 

  if (settings->m_bAttachToProcess) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, processid);
    if (hProcess) {
      BOOL bWow64Process = false;
      IsWow64Process(hProcess, &bWow64Process);
      if (dlg.m_bRunningOn64BitOS && !bWow64Process)
        dlg.m_bProfileUsingCmdLineProfiler = true;
      CloseHandle(hProcess);
    }
  } else {
    DWORD binaryType = 0;
    GetBinaryType(settings->m_executable.c_str(), &binaryType);
    if (binaryType == SCS_64BIT_BINARY) {
      dlg.m_bProfileUsingCmdLineProfiler = true;
    }
  }

  if (!dlg.m_bProfileUsingCmdLineProfiler) {
    dlg.wxThread::Create();
  }
  dlg.m_timer.wxTimer::Start(200, wxTIMER_CONTINUOUS);
  
  if (!dlg.m_bProfileUsingCmdLineProfiler) {
    dlg.wxThread::Run();
  } else {
    dlg.m_status = PrepareStatusInSharedMemory();
    if (!SampleWithCommandLineProfiler(dlg.m_settings, dlg.m_processId))
      return false;
  }
  dlg.ShowModal();  
  return dlg.m_bProfileReturnValue;
}