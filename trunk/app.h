enum
{
    Wizard_Quit = wxID_EXIT,
    Wizard_About = wxID_ABOUT,
    Wizard_RunModal = wxID_HIGHEST,
    File_Save_Settings,
    File_Save_Settings_As,
    File_Load_Settings,
    File_Save_Profile,
    File_Load_Profile,
    Edit_Find_Function,    
    Profile_Run,
    Zoom_In,
    Zoom_Out,
    MaximizeOrRestore_View,
    View_Abbreviate,
    View_Ignore_Function,
    File_Load_Source,
    Results_Grid,
    Help_ShowManual,
    HorizontalSplitter
};



class MyApp : public wxApp {
public:
    // override base class virtuals
  ~MyApp();
    virtual bool OnInit();
};

class wxTextCtrl;
class wxLog;
class MyGrid;
class Edit;
class wxSplitterWindow;
class wxToolBar;
class wxComboCtrl;
class wxListViewComboPopup;
class wxSplitterEvent;
class wxFindReplaceDialog;
class wxFindReplaceData;
class wxFindDialogEvent;

#include "CallStackView.h"

class StackWalkerMainWnd : public wxFrame, public CallStackViewClickCallback {
  ProfilerSettings m_settings;
  ProfilerSettings m_settingsBeforeWizard;
  wxTextCtrl *m_logCtrl;    
  MyGrid *m_resultsGrid;
  Edit *m_sourceEdit;
  wxSplitterWindow *m_horzSplitter;
  wxSplitterWindow *m_vertSplitter; 
  CallStackView *m_callstackView;
  wxNotebook *m_bottomNotebook;
  wxToolBar *m_toolbar;
  void ShowChildWindows(bool bshow);
  double m_zoom;
  FunctionSample *m_currentActiveFs;
  wxComboCtrl *m_toolbarThreadsCombo;
  wxListViewComboPopup *m_toolbarThreadsListPopup;  
  wxLog *m_logTargetOld;
  
  wxString m_currentFunction;
  wxString m_currentSourceFile;
  wxString m_currentDataFile;
  wxFileHistory m_fileHistory;
  int m_gridWidth;
  wxFindReplaceDialog *m_pFindReplaceDialog;
  wxFindReplaceData *m_pFindReplaceData;

  void ClearContext();
  int m_verticalSplitterRestorePosition;
  int m_horizontalSplitterRestorePosition;
  void RestoreViews();
  void RefreshGridView();
  void ProfileDataChanged();
  bool ComplainAboutNonSavedProfile();

public:
    // ctor(s)
    StackWalkerMainWnd(const wxString& title);
    ~StackWalkerMainWnd();
    void UpdateTitleBar();

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent &ev);
    void OnAbout(wxCommandEvent& event);
    void OnShowManual(wxCommandEvent& event);
    void OnRunWizard(wxCommandEvent& event);
    void OnFileSaveSettings(wxCommandEvent& event);
    void OnFileSaveSettingsAs(wxCommandEvent& event);
    void OnFileLoadSettings(wxCommandEvent& event);
    void OnFindFunction(wxCommandEvent& event);    

    void StackWalkerMainWnd::OnProfileRun(wxCommandEvent& event);
    void OnWizardCancel(wxWizardEvent& event);
    void OnWizardFinished(wxWizardEvent& event);
    void OnGridLabelLeftClick(wxGridEvent& ev);
    void OnGridSelect(wxGridEvent& ev);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnMaximizeView(wxCommandEvent& event);
    void OnViewAbbreviate(wxCommandEvent& event);
    void OnViewIgnoreFunction(wxCommandEvent& event);

    void OnFileLoadSourceFile(wxCommandEvent& event);

    void OnFileSaveProfile(wxCommandEvent& event);
    void OnFileLoadProfile(wxCommandEvent& event);
    void OnMRUFile(wxCommandEvent& event);
    

    virtual void OnClickCaller(Caller *caller);

    void ThreadSelectionChanged();
    void LoadSettings(const char *fileName);
    void LoadProfileData(const char *fileName);
    void OnHorizontalSplitterChanging(wxSplitterEvent& event);
    void OnFindDialog(wxFindDialogEvent& event);
    wxStatusBar *OnCreateStatusBar(int number, long style, wxWindowID id, const wxString& name);

private:
    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

