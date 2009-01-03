#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include "sampledata.h"
#include <wx/thread.h>
#include "profileprogress.h"
#include <wx/timer.h>
#include <wx/log.h>
#include <wx/stattext.h>

class ProfileProgressDialog : public wxDialog, public wxThread {
public:
    wxGauge *m_gauge;
    wxButton *m_cancelButton;
    wxButton *m_actionButton;
    ProfileProgressDialog(wxWindow *parent, ProfilerSettings *settings);
    ProfilerSettings *m_settings;
    ProfilerProgressStatus m_status;
    bool m_bProfileReturnValue;
    bool m_bTitleChanged;
    wxTimer m_timer;
    wxStaticText *m_staticText;

    void OnCancelButton(wxCommandEvent& event);
    void OnActionButton(wxCommandEvent& event);

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
}

void ProfileProgressDialog::OnActionButton(wxCommandEvent& WXUNUSED(event)) {
  if (!m_status.bStartedSampling) {
    m_status.bStartedSampling = true;
    return;
  }
  if (!m_status.bFinishedSampling) {
    m_status.bFinishedSampling = true;
    return;
  }
}

void ProfileProgressDialog::OnCancelButton(wxCommandEvent& WXUNUSED(event)) {
  m_status.bFinishedSampling = true;
  m_status.bStartedSampling = true;
  EndProfiling();
}


void ProfileProgressDialog::EndProfiling() {
  Wait();
  m_timer.Stop();
  EndModal(0);
}

void ProfileProgressDialog::OnClose(wxCloseEvent&) { 
  m_status.bFinishedSampling = true;
  m_status.bStartedSampling = true;
  EndProfiling(); 
}

wxThread::ExitCode ProfileProgressDialog::Entry() {
  m_bProfileReturnValue = SampleProcess(m_settings, &m_status);
  m_status.bFinishedSampling = true;
  return 0;
}

void ProfileProgressDialog::OnTimer(wxTimerEvent& WXUNUSED(evt)) {  
  if (!m_status.bStartedSampling) {
    if (m_settings->m_samplingStartDelay == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL) {
      m_staticText->SetLabel("Press 'start' to begin.");
      m_gauge->Pulse();
    } else {
      int val = m_status.secondsLeftToStart;
      m_gauge->SetRange(100 * m_settings->m_samplingStartDelay);
      m_gauge->SetValue(100 * val);
      char buf[256];
      sprintf(buf, "%d seconds to start", m_status.secondsLeftToStart);
      m_staticText->SetLabel(buf);
    }
  } else {
    if (m_settings->m_samplingTime == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL) {
      char buf[256];
      sprintf(buf, "%d samples collected", m_allThreadSamples);
      m_staticText->SetLabel(buf);
      m_gauge->Pulse();
    } else {
      int val = m_settings->m_samplingTime - m_status.secondsLeftToProfile;
      m_gauge->SetRange(100 * m_settings->m_samplingTime);
      m_gauge->SetValue(100 * val);
      char buf[256];
      sprintf(buf, "%d seconds left, %d samples collected", m_status.secondsLeftToProfile, m_allThreadSamples);
      m_staticText->SetLabel(buf);
    }
    if (!m_bTitleChanged) {
      SetLabel("Sampling...");
      m_actionButton->SetLabel("Stop Sampling");
      m_bTitleChanged = true;
    }
  }
  if (m_status.bFinishedSampling) {
    EndProfiling();
  }
}

bool SampleProcessWithDialogProgress(wxWindow *appMainWindow, ProfilerSettings *settings) {
  ProfileProgressDialog dlg(appMainWindow, settings);
  dlg.wxThread::Create();
  dlg.m_timer.wxTimer::Start(200, wxTIMER_CONTINUOUS);
  dlg.wxThread::Run();
  dlg.ShowModal();  
  return dlg.m_bProfileReturnValue;
}