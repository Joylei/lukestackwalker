#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    
    #include "wx/stattext.h"
    #include "wx/log.h"    
    #include "wx/checkbox.h"
    #include "wx/checklst.h"
    #include "wx/msgdlg.h"
    #include "wx/radiobox.h"
    #include "wx/sizer.h"
#endif

#include <wx/wizard.h>
#include <wx/valtext.h>
#include <wx/valgen.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>

#include "wiztest.xpm"
#include "wiztest2.xpm"
#include <wx/filename.h>
#include <wx/button.h>

#include "ProfilerSettings.h"
#include "sampledata.h"
#include "wizard.h"
#include <wx/filepicker.h>
#include <wx/textCtrl.h>

// ids for dialog controls items
enum {        
    Add_Dir_ID =  wxID_HIGHEST + 1,
    Rem_Dir_ID,
    Manual_Sampling_Chk_ID,
    Add_Var_ID,
    Rem_Var_ID,
    Env_Vars_ID,
    Attach_to_Process_Chk_ID,
    Stop_Sampling_Outside_Modules_ID
};



class ProfilerSettingsSamplingPage : public wxWizardPageSimple {
  ProfilerSettings *m_settings;
  wxSpinCtrl *m_samplingDepthCtrl;
  wxSpinCtrl *m_startSamplingCtrl;
  wxSpinCtrl *m_samplingDurationCtrl;
  wxCheckBox *m_manualSamplingCheckBox;
  wxCheckBox *m_connectToServerCheckBox;
  wxCheckBox *m_stopAtPCOutsideModulesCheckBox;
  wxListBox *m_debugPathsLb;
  DECLARE_EVENT_TABLE()

public:
    ProfilerSettingsSamplingPage(wxWizard *parent, ProfilerSettings *settings) : wxWizardPageSimple(parent) {
      m_settings = settings;
      m_bitmap = wxBitmap(wiztest2_xpm);

      m_samplingDepthCtrl = new wxSpinCtrl(this, wxID_ANY);
      m_startSamplingCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000);
      m_samplingDurationCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000);

      wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

      wxFlexGridSizer *subSizer = new wxFlexGridSizer(3, 5, 5);
      
      mainSizer->Add(subSizer,  0, wxALL, 5);

      subSizer->Add(new wxStaticText(this, wxID_ANY, "Stack Sampling depth:"), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

      subSizer->Add(m_samplingDepthCtrl, 0, wxTOP | wxRIGHT, 5);

      subSizer->Add(new wxStaticText(this, wxID_ANY, "frames ( 0 is unlimited ).*"), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);            
      
      subSizer->Add(new wxStaticText(this, wxID_ANY, "Start sampling after"), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

      subSizer->Add(m_startSamplingCtrl, 0, wxTOP | wxRIGHT, 5);

      subSizer->Add(new wxStaticText(this, wxID_ANY, "seconds."), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
      
      subSizer->Add( new wxStaticText(this, wxID_ANY, "Sample for"), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

      subSizer->Add(m_samplingDurationCtrl, 0, wxTOP | wxRIGHT, 5);

      subSizer->Add(new wxStaticText(this, wxID_ANY, "seconds."), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

      m_stopAtPCOutsideModulesCheckBox = new wxCheckBox(this, Stop_Sampling_Outside_Modules_ID, "Abort stack walk when address is outside known modules");
      mainSizer->Add(m_stopAtPCOutsideModulesCheckBox, 0, wxALL, 5);
      
      m_manualSamplingCheckBox = new wxCheckBox(this, Manual_Sampling_Chk_ID, "Start and stop sampling manually");
                
      mainSizer->Add(m_manualSamplingCheckBox, 0, wxALL, 5);

      m_connectToServerCheckBox = new wxCheckBox(this, wxID_ANY, "Connect to Microsoft symbol server for system symbols");

      mainSizer->Add(m_connectToServerCheckBox, 0, wxALL, 5);      

      mainSizer->AddSpacer(5);
      mainSizer->Add(new wxStaticText(this, wxID_ANY, _T("Debug info directories:")), 
                    0, wxALL, 5 );

      m_debugPathsLb = new wxListBox(this, -1, wxDefaultPosition, wxSize(350, 80));
      mainSizer->Add(m_debugPathsLb, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );

      {
        wxBoxSizer *subSizer = new wxBoxSizer(wxHORIZONTAL);          
        mainSizer->Add(subSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
        subSizer->Add(new wxButton(this, Rem_Dir_ID, _T("Remove selected directory")), 0, wxRIGHT | wxBOTTOM, 5);
        subSizer->Add(new wxButton(this, Add_Dir_ID, _T("Add new directory")), 0, wxRIGHT | wxBOTTOM, 5);
      }

      mainSizer->AddSpacer(5);
      mainSizer->Add(new wxStaticText(this, wxID_ANY, "* Use depth of 1 for best accuracy, 0 to gain a good call graph."), 0, wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);   

      SetSizer(mainSizer);
      mainSizer->Fit(this);
    }

    virtual bool TransferDataFromWindow() {
      m_settings->m_sampleDepth = m_samplingDepthCtrl->GetValue();

      if (m_manualSamplingCheckBox->IsChecked()) {
        m_settings->m_samplingStartDelay = ProfilerSettings::SAMPLINGTIME_MANUALCONTROL;
        m_settings->m_samplingTime = ProfilerSettings::SAMPLINGTIME_MANUALCONTROL;
      } else {
        m_settings->m_samplingStartDelay = m_startSamplingCtrl->GetValue();
        m_settings->m_samplingTime = m_samplingDurationCtrl->GetValue();
      }
      m_settings->m_bConnectToSymServer = m_connectToServerCheckBox->IsChecked();
      m_settings->m_bStopAtPCOutsideModules = m_stopAtPCOutsideModulesCheckBox->IsChecked();

      m_settings->m_debugInfoPaths.clear();      
      for (unsigned int i = 0; i < m_debugPathsLb->GetCount(); i++) {
        m_settings->m_debugInfoPaths.push_back(m_debugPathsLb->GetString(i));
      }

      return true;
    }

    virtual bool TransferDataToWindow() {
      m_samplingDepthCtrl->SetValue(m_settings->m_sampleDepth);

      if (m_settings->m_samplingTime == ProfilerSettings::SAMPLINGTIME_MANUALCONTROL) {
        m_manualSamplingCheckBox->SetValue(true);
        m_startSamplingCtrl->SetValue("");
        m_samplingDurationCtrl->SetValue("");
      } else {
        m_manualSamplingCheckBox->SetValue(false);
        m_startSamplingCtrl->SetValue(m_settings->m_samplingStartDelay);
        m_samplingDurationCtrl->SetValue(m_settings->m_samplingTime);
      }
      m_startSamplingCtrl->Enable(!m_manualSamplingCheckBox->IsChecked());
      m_samplingDurationCtrl->Enable(!m_manualSamplingCheckBox->IsChecked());
      m_connectToServerCheckBox->SetValue(m_settings->m_bConnectToSymServer);
      m_stopAtPCOutsideModulesCheckBox->SetValue(m_settings->m_bStopAtPCOutsideModules);

      m_debugPathsLb->Clear();
      for (std::list<wxString>::iterator it = m_settings->m_debugInfoPaths.begin();
       it != m_settings->m_debugInfoPaths.end(); ++it) {
         m_debugPathsLb->Append(*it);
      }


      return true;
    }

    void OnManualControlClicked (wxCommandEvent &) {
      m_startSamplingCtrl->Enable(!m_manualSamplingCheckBox->IsChecked());
      m_samplingDurationCtrl->Enable(!m_manualSamplingCheckBox->IsChecked());
      if (m_manualSamplingCheckBox->IsChecked()) {
        m_startSamplingCtrl->SetValue("");
        m_samplingDurationCtrl->SetValue("");
      }
    }

    void OnAddDirClicked(wxCommandEvent& ) {
      wxDirDialog dirDlg(this, "Choose a directory to load debug info from:");
      if (dirDlg.ShowModal() != wxID_OK) 
        return;
      wxString path = dirDlg.GetPath();
      m_debugPathsLb->InsertItems(1, &path, 0);
      
    }

    void OnRemoveDirClicked(wxCommandEvent& ) {
      int i = m_debugPathsLb->GetSelection();
      if (i == wxNOT_FOUND)
        return;
      m_debugPathsLb->Delete(i);
    }



private:    
};

BEGIN_EVENT_TABLE(ProfilerSettingsSamplingPage, wxWizardPageSimple)
    EVT_CHECKBOX(Manual_Sampling_Chk_ID, ProfilerSettingsSamplingPage::OnManualControlClicked)
    EVT_BUTTON(Add_Dir_ID, ProfilerSettingsSamplingPage::OnAddDirClicked)
    EVT_BUTTON(Rem_Dir_ID, ProfilerSettingsSamplingPage::OnRemoveDirClicked)
END_EVENT_TABLE()




class ProfilerSettingsTargetPage : public wxWizardPageSimple {
private:
    wxFilePickerCtrl *m_executablePicker;
    wxDirPickerCtrl *m_currDirPicker;
    wxTextCtrl *m_cmdLineArgsCtrl;
    ProfilerSettings *m_pSettings;
    wxListBox *m_envVarsLb;
    wxTextCtrl *m_currentVariable;
    wxCheckBox *m_attachToProcessCheckBox;
    wxButton *m_addVariableButton;
    wxButton *m_removeVariableButton;
    
    DECLARE_EVENT_TABLE()

public:
    // directions in which we allow the user to proceed from this page
    enum
    {
        Forward, Backward, Both, Neither
    };

    void OnAttachClicked (wxCommandEvent &) {
      m_executablePicker->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_currDirPicker->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_cmdLineArgsCtrl->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_envVarsLb->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_addVariableButton->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_removeVariableButton->Enable(!m_attachToProcessCheckBox->IsChecked());
      m_currentVariable->Enable(!m_attachToProcessCheckBox->IsChecked());
    }

    virtual bool TransferDataFromWindow() {
      m_pSettings->m_executable = m_executablePicker->GetPath();
      m_pSettings->m_commandLineArgs = m_cmdLineArgsCtrl->GetValue();
      m_pSettings->m_currentDirectory = m_currDirPicker->GetPath();
      m_pSettings->m_bAttachToProcess = m_attachToProcessCheckBox->IsChecked();

      if (m_pSettings->m_currentDirectory.empty() && !m_pSettings->m_executable.empty()) {
        wxFileName fn(m_pSettings->m_executable);
        m_pSettings->m_currentDirectory = fn.GetVolume() + fn.GetVolumeSeparator() + fn.GetPath(wxPATH_GET_SEPARATOR);
      }

      if (!m_pSettings->m_bAttachToProcess) {
        if (!wxFileName::DirExists(m_pSettings->m_currentDirectory)) {
          char buf[2048];
          sprintf(buf, "Directory [%s] does not exist, would you like to create it?", m_pSettings->m_currentDirectory.c_str());
          wxMessageDialog dlg(this, buf, "Current directory does not exist", wxYES_NO);
          if (dlg.ShowModal() == wxID_YES) {
            wxFileName::Mkdir(m_pSettings->m_currentDirectory, 0777, wxPATH_MKDIR_FULL);
          }
        }
      }

      m_pSettings->m_environmentVariables.clear();
      for (int i = 0; i < (int)m_envVarsLb->GetCount(); i++) {
        wxString var = m_envVarsLb->GetString(i);
        m_pSettings->m_environmentVariables[var.BeforeFirst('=')] = var.AfterFirst('=');
      }
      return true;
    }

    virtual bool TransferDataToWindow() {
      m_executablePicker->SetPath(m_pSettings->m_executable);
      m_cmdLineArgsCtrl->SetValue(m_pSettings->m_commandLineArgs);
      m_currDirPicker->SetPath(m_pSettings->m_currentDirectory);
      m_attachToProcessCheckBox->SetValue(m_pSettings->m_bAttachToProcess);
      m_envVarsLb->Clear();
      for (std::map<wxString, wxString>::iterator it = m_pSettings->m_environmentVariables.begin();
           it != m_pSettings->m_environmentVariables.end(); ++it) {
         wxString val = it->first + wxString("=") + it->second;
         m_envVarsLb->Insert(val, m_envVarsLb->GetCount());
      }
      wxCommandEvent dummy;
      OnAttachClicked(dummy);
      return true;
    }


    ProfilerSettingsTargetPage(wxWizard *parent, ProfilerSettings *settings) : wxWizardPageSimple(parent) {
      m_pSettings = settings;
      m_executablePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Executable to profile:", "Executable files (*.exe)|*.exe|All files (*.*)|*.*", 
                                                wxDefaultPosition, wxSize(350, 20));
            
      wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

      m_attachToProcessCheckBox = new wxCheckBox(this, Attach_to_Process_Chk_ID, "Attach to an existing process");

      mainSizer->Add(m_attachToProcessCheckBox, 0, wxALL, 5);

      mainSizer->Add(new wxStaticText(this, wxID_ANY, _T("Executable file to profile:")), 0, wxALL, 5);

      mainSizer->Add(m_executablePicker, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND | wxALIGN_TOP, 5);

      mainSizer->Add(new wxStaticText(this, wxID_ANY, _T("Command line arguments:")), 0, wxALL, 5);

      m_cmdLineArgsCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(350, 20), 0);

      mainSizer->Add(m_cmdLineArgsCtrl, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );

      mainSizer->Add(new wxStaticText(this, wxID_ANY, _T("Current Directory:")), 0, wxALL, 5);


      m_currDirPicker = new wxDirPickerCtrl(this, wxID_ANY, "", "Directory to run target executable in:", wxDefaultPosition,
                                            wxSize(350, 20), wxDIRP_USE_TEXTCTRL);

      mainSizer->Add(m_currDirPicker, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND | wxALIGN_TOP, 5);

      mainSizer->AddSpacer(5);
      mainSizer->Add(new wxStaticText(this, wxID_ANY, _T("Environment variables:")), 0, wxALL, 5 );

      m_envVarsLb = new wxListBox(this, Env_Vars_ID, wxDefaultPosition, wxSize(350, 80), 0, 0, wxLB_SORT);
      mainSizer->Add(m_envVarsLb, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );

      m_currentVariable = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxSize(350, wxDefaultCoord));
      mainSizer->Add(m_currentVariable, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );


      {
        wxBoxSizer *subSizer = new wxBoxSizer(wxHORIZONTAL);          
        mainSizer->Add(subSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
        m_removeVariableButton = new wxButton(this, Rem_Var_ID, _T("Remove variable"));
        subSizer->Add(m_removeVariableButton, 0, wxRIGHT | wxBOTTOM, 5);
        m_addVariableButton = new wxButton(this, Add_Var_ID, _T("Add/Modify variable"));
        subSizer->Add(m_addVariableButton, 0, wxRIGHT | wxBOTTOM, 5);
      }

      SetSizer(mainSizer);
      mainSizer->Fit(this);
    }

    void OnAddVarClicked(wxCommandEvent& ) {
      wxString val = m_currentVariable->GetValue();
      wxString var = val.BeforeFirst('=');
      if (var.Len() == val.Len())
        val += _T("=");
      var = var.MakeUpper();
      val = val.AfterFirst('=');
      val = var + wxString("=") + val;      
      for (int i = 0; i < (int)m_envVarsLb->GetCount(); i++) {
        wxString lbvar = m_envVarsLb->GetString(i).BeforeFirst('=');
        if (!lbvar.CmpNoCase(var)) {
          m_envVarsLb->SetString(i, val);
          return;
        }
        if (lbvar.CmpNoCase(var) > 0) {
          m_envVarsLb->Insert(val, i);
          return;
        }
      }
      m_envVarsLb->Insert(val, m_envVarsLb->GetCount());
    }

    void OnRemoveVarClicked(wxCommandEvent& ) {
      int i = m_envVarsLb->GetSelection();
      if (i == wxNOT_FOUND)
        return;
      m_envVarsLb->Delete(i);
      m_currentVariable->Clear();
    }

    void OnItemSelected(wxCommandEvent& ) {
      int sel = m_envVarsLb->GetSelection();
      if (sel == wxNOT_FOUND) {
        return;
      }      
      m_currentVariable->SetValue(m_envVarsLb->GetString(sel));      
    }

    // wizard event handlers
    void OnWizardCancel(wxWizardEvent& event) {
      if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
                        wxICON_QUESTION | wxYES_NO, this) != wxYES ) {
          event.Veto();
      }
    }

    void OnWizardPageChanging(wxWizardEvent&) {
    }

};


// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ProfilerSettingsTargetPage, wxWizardPageSimple)
    EVT_WIZARD_PAGE_CHANGING(wxID_ANY, ProfilerSettingsTargetPage::OnWizardPageChanging)
    EVT_WIZARD_CANCEL(wxID_ANY, ProfilerSettingsTargetPage::OnWizardCancel)
    EVT_BUTTON(Add_Var_ID, ProfilerSettingsTargetPage::OnAddVarClicked)
    EVT_BUTTON(Rem_Var_ID, ProfilerSettingsTargetPage::OnRemoveVarClicked)
    EVT_LISTBOX(Env_Vars_ID, OnItemSelected)
    EVT_CHECKBOX(Attach_to_Process_Chk_ID, ProfilerSettingsTargetPage::OnAttachClicked)
END_EVENT_TABLE()



// ----------------------------------------------------------------------------
// ProfilerSettingsWizard
// ----------------------------------------------------------------------------

ProfilerSettingsWizard::ProfilerSettingsWizard(wxWindow *frame, ProfilerSettings *settings, bool useSizer)
        : wxWizard(frame,wxID_ANY,_T("Profiler settings wizard"),
                   wxBitmap(wiztest_xpm),wxDefaultPosition,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_settings = settings;

    m_page1 = new ProfilerSettingsTargetPage(this, m_settings);
    ProfilerSettingsSamplingPage *page2 = new ProfilerSettingsSamplingPage(this, m_settings);

    // set the page order using a convenience function - could also use
    // SetNext/Prev directly as below    
    wxWizardPageSimple::Chain(m_page1, page2);


    if ( useSizer )
    {
        // allow the wizard to size itself around the pages
        GetPageAreaSizer()->Add(m_page1);
    }
}

