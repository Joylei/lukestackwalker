#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <wx/app.h>
#include <wx/frame.h>
#include "profilersettings.h"
#include <wx/wizard.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/log.h>
#include <wx/grid.h>
#include <wx/dc.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <wx/toolbar.h>
#include <wx/docview.h>

#include "app.h"
#include "wizard.h"
#include "sampledata.h"
#include "edit.h"
#include <wx/config.h>
#include "profileprogress.h"

#include <wx/combo.h>
#include <wx/listctrl.h>
#include <wx/tooltip.h>
#include <wx/file.h>
#include <wx/fdrepdlg.h>
#include <wx/generic/statusbr.h>
#include <wx/settings.h>



static wxTextCtrl *s_pLogCtrl = 0;



class wxListViewComboPopup : public wxListView, public wxComboPopup {
public:
  // Create popup control

  virtual bool Create(wxWindow* parent) {    
    bool ret = wxListView::Create(parent,1,wxPoint(0,0),wxDefaultSize, wxLC_SMALL_ICON);
    wxToolTip *m_tt = new wxToolTip("Use Ctrl+left-click to select multiple threads.");
    m_tt->SetDelay(400);
    m_tt->Enable(true);    
    SetToolTip(m_tt);
    return ret;
  }

  // Return pointer to the created control
  virtual wxWindow *GetControl() { return this; }

  // Translate string into a list selection
  virtual void SetStringValue(const wxString& s) {
    for (int i = 0; i < wxListView::GetItemCount(); i++) {
      if (s == "All") {
        wxListView::Select(i);
      } else {
        wxString label = wxListView::GetItemText(i);
        label = label.BeforeFirst(' ');
        wxString l2 = wxString(" ") + label;
        label += wxString(" ");
        if (s == label || s.Contains(label) || s.Contains(l2)) {
          wxListView::Select(i, true);
        } else {
          wxListView::Select(i, false);
        }        
      }
    }
  }

  // Get list selection as a string
  virtual wxString GetStringValue() const {
    int item = wxListView::GetFirstSelected();
    if (wxListView::GetSelectedItemCount() == 1) {
      return wxListView::GetItemText(item);
    }
    wxString str;
    double cpuTime = 0;
    while (item >= 0) {
      wxString s2 = wxListView::GetItemText(item);
      unsigned int threadId = 0;
      s2 = s2.BeforeFirst(' ');
      sscanf(s2, " 0x%x", &threadId);
      for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin(); it != g_threadSamples.end(); it++) {
        if (it->first == threadId)
          cpuTime += it->second.GetCPUTime_ms()/1000.0;
      }      
      str += s2.BeforeFirst(' ') + wxString(" ");
      item = wxListView::GetNextSelected(item);
    }
    char buf[256];
    sprintf(buf, "- %0.2lfs CPU time", cpuTime);
    if (!str.IsEmpty())
      str += buf;
    return str;
  }



  void OnMouseClick(wxMouseEvent&) {
    if (m_bDismissOnLButtonRelease) {
      Dismiss();
    }
  }

  
  void OnListSelectionChanged(wxListEvent&) {    
    m_combo->SetValue(GetStringValue());
    m_bChanged = true;
    m_bDismissOnLButtonRelease = false;
    if (!wxGetKeyState(WXK_CONTROL) && !wxGetKeyState(WXK_SHIFT)) {
      m_bDismissOnLButtonRelease = true;
      Dismiss();
    }
  }  

  void OnItemDeselected(wxListEvent&) {
    wxString stringValue = GetStringValue();
    if (!stringValue.empty()) 
      m_combo->SetValue(stringValue);
    m_bChanged = true;    
  }

  void OnPopup(void) {
    m_bChanged = false;
    m_tt->Enable(wxListView::GetItemCount() > 1);
  }

  void OnDismiss(void) {
    if (m_bChanged) {
      SetStringValue(m_combo->GetValue());
      m_owner->ThreadSelectionChanged();
    }
  }
  

  void SetOwner(StackWalkerMainWnd *newOwner) {
    m_owner = newOwner;
  }

  void EnableTooltip(bool bEnable) {m_tt->Enable(bEnable);}

protected:
   bool m_bChanged;
   bool m_bDismissOnLButtonRelease;
   StackWalkerMainWnd *m_owner;
   wxToolTip *m_tt;

private:
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxListViewComboPopup, wxListView)
EVT_LIST_ITEM_SELECTED(wxID_ANY, wxListViewComboPopup::OnListSelectionChanged)
EVT_LIST_ITEM_DESELECTED(wxID_ANY, wxListViewComboPopup::OnItemDeselected)
EVT_RIGHT_UP(wxListViewComboPopup::OnMouseClick)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(StackWalkerMainWnd, wxFrame)
EVT_MENU(Wizard_Quit,             StackWalkerMainWnd::OnQuit)
EVT_MENU(Wizard_About,            StackWalkerMainWnd::OnAbout)
EVT_MENU(Help_ShowManual,            StackWalkerMainWnd::OnShowManual)
EVT_MENU(Wizard_RunModal,         StackWalkerMainWnd::OnRunWizard)
EVT_MENU(File_Save_Settings,      StackWalkerMainWnd::OnFileSaveSettings)
EVT_MENU(File_Save_Settings_As,   StackWalkerMainWnd::OnFileSaveSettingsAs)
EVT_MENU(File_Load_Settings,      StackWalkerMainWnd::OnFileLoadSettings)
EVT_MENU(File_Load_Source,        StackWalkerMainWnd::OnFileLoadSourceFile)

EVT_MENU(Edit_Find_Function,      StackWalkerMainWnd::OnFindFunction)

EVT_MENU(View_Abbreviate,         StackWalkerMainWnd::OnViewAbbreviate)
EVT_MENU(Profile_Run,             StackWalkerMainWnd::OnProfileRun)
EVT_MENU(Zoom_Out,                StackWalkerMainWnd::OnZoomOut)
EVT_MENU(Zoom_In,                 StackWalkerMainWnd::OnZoomIn)
EVT_MENU(MaximizeOrRestore_View,  StackWalkerMainWnd::OnMaximizeView)
EVT_MENU(View_Ignore_Function,    StackWalkerMainWnd::OnViewIgnoreFunction)

EVT_MENU(View_SamplesAsPercentage, StackWalkerMainWnd::OnViewSamplesAsPercentage)
EVT_UPDATE_UI(View_SamplesAsPercentage, StackWalkerMainWnd::OnUpdateMenuItemSamplesAsPercentage)
EVT_MENU(View_SamplesAsSampleCounts, StackWalkerMainWnd::OnViewSamplesAsSampleCounts)
EVT_UPDATE_UI(View_SamplesAsSampleCounts, StackWalkerMainWnd::OnUpdateMenuItemSamplesAsSampleCounts)

EVT_MENU(File_Save_Profile,      StackWalkerMainWnd::OnFileSaveProfile)
EVT_MENU(File_Load_Profile,      StackWalkerMainWnd::OnFileLoadProfile)


EVT_WIZARD_CANCEL(wxID_ANY,   StackWalkerMainWnd::OnWizardCancel)
EVT_WIZARD_FINISHED(wxID_ANY, StackWalkerMainWnd::OnWizardFinished)

EVT_GRID_LABEL_LEFT_CLICK(StackWalkerMainWnd::OnGridLabelLeftClick)
EVT_GRID_CELL_LEFT_CLICK(StackWalkerMainWnd::OnGridLabelLeftClick)  
EVT_GRID_SELECT_CELL(StackWalkerMainWnd::OnGridSelect)  

EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, StackWalkerMainWnd::OnMRUFile)

EVT_CLOSE(StackWalkerMainWnd::OnClose)

EVT_SPLITTER_SASH_POS_CHANGING(HorizontalSplitter, StackWalkerMainWnd::OnHorizontalSplitterChanging)  

EVT_FIND(wxID_ANY, StackWalkerMainWnd::OnFindDialog)
EVT_FIND_NEXT(wxID_ANY, StackWalkerMainWnd::OnFindDialog)


END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)


// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// `Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
  wxConfig *config = new wxConfig("Luke StackWalker");
  wxConfig::Set(config);

  StackWalkerMainWnd *frame = new StackWalkerMainWnd(_T("Luke StackWalker"));

  // and show it (the frames, unlike simple controls, are not shown when
  // created initially)
  frame->Show(true);

  if (argc > 1) {
    wxFileName fn(argv[1]);
    wxString ext = fn.GetExt();
    if (!ext.CmpNoCase("lsp")) {
      frame->LoadSettings(argv[1]);
    }
    if (!ext.CmpNoCase("lsd")) {
      frame->LoadProfileData(argv[1]);      
    }
  }
  frame->UpdateTitleBar();

  // we're done
  return true;
}

MyApp::~MyApp() {
}

// ----------------------------------------------------------------------------
// StackWalkerMainWnd
// ----------------------------------------------------------------------------
class MyGrid : public wxGrid {
public:
  MyGrid(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS, const wxString& name = wxPanelNameStr) : 
    wxGrid(parent, id, pos, size, style, name) {
    void SetCursorMode();
  };
  void SetCursorMode() {    
    ChangeCursorMode(wxGrid::WXGRID_CURSOR_SELECT_ROW);
  }
  void OnKeyDown(wxKeyEvent &evt) {
    if (evt.GetKeyCode() == WXK_DOWN) {
      wxArrayInt rows = GetSelectedRows();
      if (rows.Count()) {
        if (rows.Item(0) < GetNumberRows() - 1) {
          SelectRow(rows.Item(0) + 1);
          SetGridCursor(rows.Item(0) + 1, 0);
          MakeCellVisible(rows.Item(0) + 1, 0);
        }
      } else {
        SelectRow(0);
        SetGridCursor(0, 0);
        MakeCellVisible(0, 0);
      }
    }
    if (evt.GetKeyCode() == WXK_UP) {
      wxArrayInt rows = GetSelectedRows();
      if (rows.Count() && rows.Item(0)) {
        SelectRow(rows.Item(0) - 1);
        SetGridCursor(rows.Item(0) - 1, 0);
        MakeCellVisible(rows.Item(0) - 1, 0);
      } else {
        SelectRow(0);
        SetGridCursor(0, 0);
        MakeCellVisible(0, 0);
      }
    }
  }


  void OnMouseMove(wxMouseEvent &) {
  }
  
  DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MyGrid, wxGrid)
EVT_KEY_DOWN(MyGrid::OnKeyDown) 
EVT_MOTION(MyGrid::OnMouseMove) 
EVT_LEFT_DOWN(MyGrid::OnMouseMove)  
END_EVENT_TABLE()

class MyStatusBar : public wxStatusBarGeneric {
public:
  MyStatusBar(wxFrame *frame, wxWindowID id, long style) : wxStatusBarGeneric(frame, id, style) {
  };
  virtual void SetStatusText(const wxString& text, int i = 0) {
    wxString str = text;
    if (text.length() < 3) {      
      str = " ";
    }
    wxStatusBarGeneric::SetStatusText(str, i);
  }
};

wxStatusBar *StackWalkerMainWnd::OnCreateStatusBar(int number,
                                        long style,
                                        wxWindowID id,
                                        const wxString&)
{
    wxStatusBar *statusBar wxDUMMY_INITIALIZE(NULL);

    statusBar = (wxStatusBar *)new MyStatusBar(this, id, style);

    statusBar->SetFieldsCount(number);

    return statusBar;
}


void StackWalkerMainWnd::OnMRUFile(wxCommandEvent& ev) {
  wxFileName fn(m_fileHistory.GetHistoryFile(ev.GetId() - wxID_FILE1));
  if (!fn.FileExists()) {
    wxString str = "The file [";
    str += m_fileHistory.GetHistoryFile(ev.GetId() - wxID_FILE1);
    str += "] could not be opened.";
    wxMessageBox(str, wxT("Error"), wxOK | wxCENTRE | wxICON_HAND);
    return;
  }
  wxString ext = fn.GetExt();
  if (!ext.CmpNoCase("lsp")) {
    LoadSettings(fn.GetFullPath());
  }
  if (!ext.CmpNoCase("lsd")) {
    if (ComplainAboutNonSavedProfile()) {
      return;
    }
    LoadProfileData(fn.GetFullPath());
  }
}

void StackWalkerMainWnd::ShowChildWindows(bool bShow) {
  m_logCtrl->Show(bShow);  
  m_resultsGrid->Show(bShow);
  m_sourceEdit->Show(bShow);
  m_horzSplitter->Show(bShow);
  m_vertSplitter->Show(bShow);
  if (bShow) {
    Refresh();
    Update();
  }
  SendSizeEvent();
}

StackWalkerMainWnd::StackWalkerMainWnd(const wxString& title)
:wxFrame((wxFrame *)NULL, wxID_ANY, title,
         wxDefaultPosition, wxSize(600, 400))  // small frame
{

  wxMenu *menuFile = new wxMenu;

  m_pFindReplaceDialog = 0;
  m_pFindReplaceData =  new wxFindReplaceData(wxFR_DOWN);

  wxLog::SetTimestamp(0);
  wxLog::SetLogLevel(1000);


  menuFile->Append(File_Save_Settings, _T("Save Project Settings...\tCtrl-S"));
  menuFile->Append(File_Save_Settings_As, _T("Save Project Settings As..."));
  menuFile->Append(File_Load_Settings, _T("Load Project Settings..."));
  menuFile->AppendSeparator();
  menuFile->Append(File_Load_Source, _T("Load Source File..."), _T("Load a source file from a location that you can specify - useful if the file cannot be found automatically."));
  menuFile->AppendSeparator();
  menuFile->Append(File_Save_Profile, _T("Save Profile Data..."), _T("Save profile data to a file."));
  menuFile->Append(File_Load_Profile, _T("Load Profile Data..."), _T("Load previously saved profile data from a file."));
  menuFile->AppendSeparator();
  menuFile->Append(Wizard_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));
  m_fileHistory.UseMenu(menuFile);
  m_fileHistory.AddFilesToMenu();

  wxMenu *menuEdit = new wxMenu;
  menuEdit->Append(Edit_Find_Function, "&Find\tCtrl-F", "Find a function in the profile.");
  
  

  wxMenu *profileMenu = new wxMenu;
  profileMenu->Append(Wizard_RunModal, _T("&Project Setup...\tCtrl-R"), _T("Starts the project setup wizard."));
  profileMenu->Append(Profile_Run, "Run\tF5", "Run a new profiling session");

  m_viewMenu = new wxMenu;
  m_viewMenu->Append(Zoom_In, _T("Zoom &In\tCtrl-I"), _T("Zoom in call graph view."));
  m_viewMenu->Append(Zoom_Out, _T("Zoom &Out\tCtrl-O"), _T("Zoom out call graph view."));
  m_viewMenu->Append(MaximizeOrRestore_View, _T("&Maximize/Restore Current View\tCtrl-M"));
  m_viewMenu->Append(View_Abbreviate, _T("&Abbreviate Name\tCtrl-A"), _T("Specify abbreviations to shorten displayed names."));
  m_viewMenu->Append(View_Ignore_Function, _T("&Ignore/Count in This function\tF2"), _T("Toggles whether this function is included in total sample count calculation."));
  
  m_viewMenu->AppendRadioItem(View_SamplesAsPercentage, _T("Show Samples as &Percentages\tCtrl-P"), _T("Show sample counts as percentages of total samples in the selected thread(s)."));
  m_viewMenu->AppendRadioItem(View_SamplesAsSampleCounts, _T("Show Sample Counts\tCtrl-K"), _T("Show straight sample counts."));
  m_viewMenu->FindItem(View_SamplesAsPercentage)->Check(true);

  wxMenu *helpMenu = new wxMenu;
  helpMenu->Append(Help_ShowManual, _T("&User's Manual...\tF1"), _T("Show Luke Stackwalker user's manual (requires a PDF reader application)."));
  helpMenu->Append(Wizard_About, _T("&About..."), _T("Show about dialog"));

  // now append the freshly created menu to the menu bar...
  wxMenuBar *menuBar = new wxMenuBar();
  menuBar->Append(menuFile, _T("&File"));
  menuBar->Append(menuEdit, _T("&Edit"));
  menuBar->Append(profileMenu, "&Profile");
  menuBar->Append(m_viewMenu, "&View");
  menuBar->Append(helpMenu, _T("&Help"));

  // ... and attach this menu bar to the frame
  SetMenuBar(menuBar);

  wxConfigBase::Get()->SetPath(_T("/MainFrame"));

  UseNativeStatusBar(false);
  CreateStatusBar();
  int widths = -1;
  GetStatusBar()->SetFieldsCount(1);
  GetStatusBar()->SetStatusWidths(1, &widths);  


  enum TbBitMaps {OPEN, SAVE_SETTINGS, SAVE_PROFILE, SETTINGS, RUN, ZOOM_IN, ZOOM_OUT, NBITMAPS};
  static wxBitmap toolBarBitmaps[NBITMAPS];

#define INIT_TOOL_BMP(bmp) toolBarBitmaps[bmp] = wxBitmap(#bmp)

  INIT_TOOL_BMP(OPEN);
  INIT_TOOL_BMP(SAVE_SETTINGS);
  INIT_TOOL_BMP(SAVE_PROFILE);
  INIT_TOOL_BMP(SETTINGS);
  INIT_TOOL_BMP(RUN);
  INIT_TOOL_BMP(ZOOM_IN);
  INIT_TOOL_BMP(ZOOM_OUT);

  m_toolbar = CreateToolBar();

  m_toolbar->SetToolBitmapSize(wxSize(16,16));

  m_toolbar->AddTool(File_Load_Settings, "Load Settings",
    toolBarBitmaps[OPEN], wxNullBitmap, wxITEM_NORMAL,
    "Load profiler project settings");

  m_toolbar->AddTool(File_Save_Settings, "Save Settings",
    toolBarBitmaps[SAVE_SETTINGS], wxNullBitmap, wxITEM_NORMAL,
    "Save profiler project settings");

  m_toolbar->AddTool(File_Save_Profile, "Save Profile Data",
    toolBarBitmaps[SAVE_PROFILE], wxNullBitmap, wxITEM_NORMAL,
    "Save profile data");


  m_toolbar->AddTool(Wizard_RunModal, "Project Setup",
    toolBarBitmaps[SETTINGS], wxNullBitmap, wxITEM_NORMAL,
    "Run project setup wizard.");

  m_toolbar->AddTool(Profile_Run, "Run profile",
    toolBarBitmaps[RUN], wxNullBitmap, wxITEM_NORMAL,
    "Run a new profiling session.");

  m_toolbar->AddTool(Zoom_In, "Zoom In",
    toolBarBitmaps[ZOOM_IN], wxNullBitmap, wxITEM_NORMAL,
    "Zoom in call graph view.");

  m_toolbar->AddTool(Zoom_Out, "Zoom Out",
    toolBarBitmaps[ZOOM_OUT], wxNullBitmap, wxITEM_NORMAL,
    "Zoom out call graph view.");

  wxStaticText *threadPrompt = new wxStaticText(m_toolbar, wxID_ANY, "  Threads: ");
  m_toolbar->AddControl(threadPrompt);

  m_toolbarThreadsCombo = new wxComboCtrl(m_toolbar,wxID_ANY,wxEmptyString, wxDefaultPosition, wxSize(300,wxDefaultCoord), wxCB_READONLY);
  m_toolbarThreadsCombo->SetPopupMaxHeight(200);

  m_toolbarThreadsListPopup = new wxListViewComboPopup();
  m_toolbarThreadsListPopup->SetOwner(this);

  m_toolbarThreadsCombo->SetPopupControl(m_toolbarThreadsListPopup);
  
  m_toolbar->AddControl(m_toolbarThreadsCombo);


  m_toolbar->Realize();

  // restore frame position and size
  int x = wxConfigBase::Get()->Read(_T("x"), 50),
    y = wxConfigBase::Get()->Read(_T("y"), 50),
    w = wxConfigBase::Get()->Read(_T("w"), 350),
    h = wxConfigBase::Get()->Read(_T("h"), 200);
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  int sxs = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
  int sys = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
  if (x >= sxs)
    x = sxs - 100;
  if (y >= sys)
    y = sys - 100;

  Move(x, y);
  SetClientSize(w, h);


  m_vertSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, GetClientSize(), wxSP_3D | wxSP_BORDER);

  m_bottomNotebook = new wxNotebook(m_vertSplitter, wxID_ANY,
    wxDefaultPosition, wxDefaultSize, wxNB_TOP);

  m_logCtrl = new wxTextCtrl(m_bottomNotebook, wxID_ANY, wxEmptyString,
    wxDefaultPosition, wxDefaultSize,
    wxTE_RICH | wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE | wxHSCROLL);
  
  s_pLogCtrl = m_logCtrl;

  m_bottomNotebook->AddPage( m_logCtrl, "Log", false);

  m_callstackView = new CallStackView(m_bottomNotebook, &m_settings);
  m_callstackView->SetClickCallback(this);

  m_bottomNotebook->AddPage( m_callstackView , "Callers", false);


  m_horzSplitter = new wxSplitterWindow(m_vertSplitter, HorizontalSplitter, wxDefaultPosition, GetClientSize(), wxSP_3D | wxSP_BORDER);
  

  m_editParent = new EditParent(m_horzSplitter, wxID_ANY);

  m_sourceEdit = m_editParent->GetEdit();
  
  m_resultsGrid = new MyGrid( m_horzSplitter,
    Results_Grid,
    wxDefaultPosition, wxDefaultSize);

  m_vertSplitter->SplitHorizontally(m_horzSplitter, m_bottomNotebook);  
  m_horzSplitter->SplitVertically(m_resultsGrid, m_editParent);

  m_vertSplitter->UpdateSize();
  m_vertSplitter->SetMinimumPaneSize(1);  
  m_horzSplitter->UpdateSize();
  m_horzSplitter->SetMinimumPaneSize(1);

  ShowChildWindows(false);

  SetIcon(wxICON(APPICON));
  m_zoom = 1.0;
  m_currentActiveFs = 0;
  m_verticalSplitterRestorePosition = -1;
  m_horizontalSplitterRestorePosition = -1;
  m_resultsGrid->SetRowLabelSize(0);
  m_fileHistory.Load(*wxConfigBase::Get());
  m_gridWidth = 0;

  m_logTargetOld = wxLog::SetActiveTarget( new wxLogTextCtrl(m_logCtrl)  );

  m_bWievSamplesAsSampleCounts = !!wxConfigBase::Get()->Read(_T("showSampleCounts"), (long)0);
  m_sourceEdit->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
  m_callstackView->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
}

StackWalkerMainWnd::~StackWalkerMainWnd() {
  delete m_pFindReplaceDialog;
  delete m_pFindReplaceData;  

  if (!IsMaximized()) {
    int x, y, w, h;
    GetClientSize(&w, &h);
    GetPosition(&x, &y);
    wxConfigBase::Get()->Write(_T("/MainFrame/x"), (long) x);
    wxConfigBase::Get()->Write(_T("/MainFrame/y"), (long) y);
    wxConfigBase::Get()->Write(_T("/MainFrame/w"), (long) w);
    wxConfigBase::Get()->Write(_T("/MainFrame/h"), (long) h);
  }

  wxConfigBase::Get()->Write(_T("/MainFrame/showSampleCounts"), (long)m_bWievSamplesAsSampleCounts);

  m_fileHistory.Save(*wxConfigBase::Get());

  delete wxLog::SetActiveTarget( m_logTargetOld  );
  wxLog::SetLogLevel(0);
  
}


bool StackWalkerMainWnd::ComplainAboutNonSavedProfile() {
  if (!g_bNewProfileData)
    return false;
  wxMessageDialog dlg(this, _("The profile data has not been saved.\nDo you want to save before continuing?"), _("Note"), wxYES_NO|wxCANCEL);
  int ret = dlg.ShowModal();
  if (ret == wxID_CANCEL) {
    return true;
  }
  if (ret == wxID_YES) {
    wxCommandEvent ev2;
    OnFileSaveProfile(ev2);    
  }
  return false;
}

void StackWalkerMainWnd::OnClose(wxCloseEvent &ev) {
  if (!ev.CanVeto())
    return;
  if (m_settings.m_bChanged) {
    wxMessageDialog dlg(this, _("The project settings have changed.\nDo you want to save them before closing Luke Stackwalker?"), _("Note"), wxYES_NO|wxCANCEL);
    int ret = dlg.ShowModal();
    if (ret == wxID_CANCEL) {
      ev.Veto();
      return;
    }
    if (ret == wxID_YES) {
      wxCommandEvent ev2;
      OnFileSaveSettingsAs(ev2);
      return;
    }
  }
  if (ComplainAboutNonSavedProfile()) {
    ev.Veto();
    return;
  }
  ev.Skip();
}

void StackWalkerMainWnd::OnQuit(wxCommandEvent&) {
  Close(false);
}

#include "lukesw-version.h"

void StackWalkerMainWnd::OnAbout(wxCommandEvent& WXUNUSED(event)) {

  char buf[2048];
  sprintf(buf,
    _T("Luke StackWalker version %d.%d.%d - a win32 & x64 profiler (c) 2008-2010 Sami Sallinen, licensed under the BSD license\n\n\n")
    _T("Included third party software:\n\n")
    _T("Walking the callstack (http://www.codeproject.com/KB/threads/StackWalker.aspx) - source code (c) 2005-2007 Jochen Kalmbach,\nlicensed under the BSD license\n\n")
    _T("Graphviz library (c) 1994-2004 AT&T Corp, licensed under the Common Public License\n\n")
    _T("WxWidgets library Copyright (c) 1998-2005 Julian Smart, Robert Roebling et al, licensed under the wxWindows Library Licence\n\n")
    _T("Silk Icons by Mark James http://www.famfamfam.com/lab/icons/silk/, licensed under the Creative Commons Attribution 2.5 License\n\n")
    _T("Microsoft debugging tools for Windows redistributable components, redistributed under 'MICROSOFT SOFTWARE LICENSE TERMS'\n"),
    
    VERSION_MAJOR, VERSION_MINOR, VERSION_BUGFIX);

  wxMessageBox(buf, _T("About Luke StackWalker"), wxOK | wxICON_INFORMATION, this);
}

void StackWalkerMainWnd::OnShowManual(wxCommandEvent& WXUNUSED(event)) {
  char moduleFileName[1024] = {0};
  GetModuleFileName(0, moduleFileName, sizeof(moduleFileName));
  wxString fn = moduleFileName;
  fn = fn.BeforeLast('\\') + wxString("\\luke stackwalker manual.pdf");
  ShellExecute(0, "open", fn.c_str(), "", "", SW_SHOWNORMAL);
}

void StackWalkerMainWnd::OnRunWizard(wxCommandEvent& WXUNUSED(event)) {
  m_settingsBeforeWizard = m_settings;
  ProfilerSettingsWizard wizard(this, &m_settings);
  wizard.RunWizard(wizard.GetFirstPage());
  if (m_settings.m_currentDirectory.empty()) {
    wxFileName fn(m_settings.m_executable);
    m_settings.m_currentDirectory = fn.GetVolume() + fn.GetVolumeSeparator() + fn.GetPath(wxPATH_GET_SEPARATOR);
    m_settings.m_bChanged = TRUE;
    UpdateTitleBar();
  }
}


wxString StripPath(const wxString &srcFileName) {
  wxFileName fn(srcFileName); 
  return fn.GetName() + wxString(".") + fn.GetExt();
}

void StackWalkerMainWnd::UpdateTitleBar() {

  wxString label = "Luke Stackwalker";

  if (m_settings.m_settingsFileName.length()) {
    label += " - ";
    label += StripPath(m_settings.m_settingsFileName);
    if (m_settings.m_bChanged) {
      label += "*";
    }
  }

  if (m_currentDataFile.length()) {
    label += " - ";
    label += StripPath(m_currentDataFile);
  }

  if (m_currentSourceFile.length()) {
    label += " - ";
    label += StripPath(m_currentSourceFile);
  }

  if (m_currentFunction.length()) {
    label += " - ";
    label += m_settings.DoAbbreviations(m_currentFunction);
  }

  SetLabel(label);
}


void StackWalkerMainWnd::OnFileSaveSettings(wxCommandEvent& ev) {
  if (!m_settings.m_settingsFileName.Length()) {
    OnFileSaveSettingsAs(ev);
    UpdateTitleBar();
    return;
  }
  if (!m_settings.SaveAs(m_settings.m_settingsFileName.c_str())) {
    wxMessageBox(wxT("Saving the settings failed."), wxT("Notification"));
  } else {
    UpdateTitleBar();
  }  
}

void StackWalkerMainWnd::OnFileSaveSettingsAs(wxCommandEvent& WXUNUSED(event)) {
  wxFileDialog fdlg(this, "Save settings as:",
    "", "", "Luke StackWalker project files (*.lsp)|*.lsp", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (fdlg.ShowModal() == wxID_CANCEL)
    return;
  if (!m_settings.SaveAs(fdlg.GetPath().c_str())) {
    wxMessageBox(wxT("Saving the settings failed."), wxT("Notification"));
  } else {
    m_fileHistory.AddFileToHistory(fdlg.GetPath());
  }
  m_settings.m_settingsFileName = fdlg.GetPath();  
  UpdateTitleBar();
}

void StackWalkerMainWnd::LoadSettings(const char *fileName) {
  if (ComplainAboutNonSavedProfile()) {
    return;
  }
  g_threadSamples.clear();
  g_displayedSampleInfo = 0;
  m_currentDataFile = "";

  if (!m_settings.Load(fileName)) {
    wxMessageBox(wxT("Loading the settings failed."), wxT("Notification"));
    m_settings.m_settingsFileName = "";
  } else {
    m_settings.m_settingsFileName = fileName;  
    m_fileHistory.AddFileToHistory(fileName);
  }
  ClearContext();
  ProfileDataChanged();
  UpdateTitleBar();
}

void StackWalkerMainWnd::OnFileLoadSettings(wxCommandEvent& WXUNUSED(event)) {
  wxFileDialog fdlg(this, "Select a project settings file to load",
    "", "", "Luke StackWalker project files (*.lsp)|*.lsp");
  if (fdlg.ShowModal() == wxID_CANCEL)
    return;
  LoadSettings(fdlg.GetPath().c_str());
}

void StackWalkerMainWnd::OnFileLoadSourceFile(wxCommandEvent&) {
  wxString title = "Find source file ";
  title += m_currentSourceFile;
  wxFileDialog fdlg(this, title,
    "", m_currentSourceFile);
  if (fdlg.ShowModal() == wxID_CANCEL)
    return;

  m_sourceEdit->SetReadOnly(false);
  m_sourceEdit->ClearAll();
  if (m_sourceEdit->LoadFile(m_currentSourceFile, fdlg.GetPath())) {
    m_settings.m_sourceFileSubstitutions[m_currentSourceFile] = fdlg.GetPath();
    m_settings.m_bChanged = TRUE;
    UpdateTitleBar();
  }
  m_sourceEdit->SetReadOnly(true);
}

void StackWalkerMainWnd::OnClickCaller(Caller *caller) {
  m_sourceEdit->SetReadOnly(false);
  m_sourceEdit->Freeze();
  if ((m_sourceEdit->GetFilename() != caller->m_functionSample->m_fileName.c_str()) || caller->m_functionSample->m_fileName.empty()) {
    m_sourceEdit->ClearAll();
    m_sourceEdit->SetFilename("");
    wxString fn = caller->m_functionSample->m_fileName.c_str();
    if (fn.IsEmpty()) {
      m_currentSourceFile = "";
      m_sourceEdit->LoadFile("");
    } else {
      if (m_settings.m_sourceFileSubstitutions.find(fn) != m_settings.m_sourceFileSubstitutions.end()) {
        m_sourceEdit->LoadFile(fn, m_settings.m_sourceFileSubstitutions.find(fn)->second);
      } else {
        m_sourceEdit->LoadFile(fn);
      }
      m_currentSourceFile = m_sourceEdit->GetFilename();
    }
  }

  int line = caller->m_lineNumber;

  m_currentFunction = caller->m_functionSample->m_functionName;

  if (caller->m_functionSample == m_currentActiveFs) {
    int maxSampleCount = 0;
    line = (m_currentActiveFs->m_minLine + m_currentActiveFs->m_maxLine) / 2;
    std::map<std::string, FileLineInfo>::iterator it =  g_displayedSampleInfo->m_lineSamples.find(caller->m_functionSample->m_fileName.c_str());

    if (it != g_displayedSampleInfo->m_lineSamples.end()) {
      FileLineInfo *pfli = &it->second;
      for (int l = m_currentActiveFs->m_minLine; l <= m_currentActiveFs->m_maxLine; l++) {
        if (((int)pfli->m_lineSamples.size() > l) && (pfli->m_lineSamples[l].m_sampleCount > maxSampleCount)) {
          maxSampleCount = pfli->m_lineSamples[l].m_sampleCount;
          line = l;
        }
      }
    }
  }

  m_sourceEdit->GotoLine(caller->m_functionSample->m_maxLine);
  m_sourceEdit->GotoLine(caller->m_functionSample->m_minLine);  
  int start = m_sourceEdit->PositionFromLine(line-1);
  int end = m_sourceEdit->PositionFromLine(line);
  m_sourceEdit->SetSelection(start, end);
  m_sourceEdit->ShowLineNumbers();
  m_sourceEdit->SetReadOnly(true);
  m_sourceEdit->Thaw();
  UpdateTitleBar();
}

void StackWalkerMainWnd::OnGridSelect(wxGridEvent &ev) {
  int row = ev.GetRow();
  ev.Skip();



  int r = g_displayedSampleInfo->m_sortedFunctionSamples.size() - 1;
  for (std::list<FunctionSample *> ::iterator it = g_displayedSampleInfo->m_sortedFunctionSamples.begin();
    it != g_displayedSampleInfo->m_sortedFunctionSamples.end(); ++it) {
      if (r == row) {        
        m_currentActiveFs = (*it);
        m_callstackView->ShowCallstackToFunction((*it)->m_functionName.c_str(), m_settings.m_bStopAtPCOutsideModules);
        if (m_currentActiveFs->m_callgraph.size()) {
          OnClickCaller(&(*m_currentActiveFs->m_callgraph.begin()));
        }
        break;        
      }
      r--;
  }
  
  if (m_currentActiveFs && m_currentActiveFs->m_functionName.length()) { 
    wxString prompt = m_settings.DoAbbreviations(m_currentActiveFs->m_functionName);
    prompt += " samples in all threads: ";
    std::multimap<unsigned int, unsigned int> samplesInThreads; 
    // first insert the per-thread sample counts to a multimap to get them sorted by sample count
    for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin(); it != g_threadSamples.end(); ++it) {
      std::map<std::string, FunctionSample>::iterator fsit = it->second.m_functionSamples.find(m_currentActiveFs->m_functionName);
      if (fsit != it->second.m_functionSamples.end()) {
        samplesInThreads.insert(std::pair<unsigned int, unsigned int>(fsit->second.m_sampleCount, it->first));        
      }
    }
    // then add them in order to the status string
    wxString thlist;
    for (std::multimap<unsigned int, unsigned int>::iterator it = samplesInThreads.begin(); it != samplesInThreads.end(); ++it) {
      if (thlist.length()) {
        thlist = ", " + thlist;
      }
      char buf[256];
      sprintf(buf, "0x%X:%d", it->second, it->first);
      thlist = buf + thlist;
    }
    prompt += thlist;
    GetStatusBar()->SetStatusText(prompt);
  }
}


void StackWalkerMainWnd::OnGridLabelLeftClick(wxGridEvent &ev) {
  int row = ev.GetRow();
  if (row < 0) {
    return;
  }
  m_resultsGrid->ClearSelection();
  m_resultsGrid->SelectRow(row);
  if (m_resultsGrid->GetGridCursorRow() != row) 
    m_resultsGrid->SetGridCursor(row, 0);

}

class ProfilerGridCellRenderer : public wxGridCellStringRenderer {
public:
  static double m_maxValue;
  virtual wxGridCellRenderer *Clone() const { 
    return new ProfilerGridCellRenderer; 
  }

  void Draw(wxGrid& grid, wxGridCellAttr& attr,
    wxDC& dc, const wxRect& rectCell,
    int row, int col, bool isSelected) {
      wxRect rect = rectCell;
      rect.Inflate(-1);

      // erase only this cells background, overflow cells should have been erased
      wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
      wxRect rectBar = rect;
      rectBar.Inflate(-2);
      dc.SetPen(*wxWHITE_PEN);
      dc.SetBrush(*wxGREEN_BRUSH);
      double perc = atof(grid.GetCellValue(row, col).c_str()) / m_maxValue;

      rectBar.width = (int)(rectBar.width * perc);

      wxColor ec = *wxRED;
      wxColor sc = *wxGREEN;
      wxColour barEndC = GetGradientEndColorByFraction(sc, ec, perc);                    
      dc.GradientFillLinear(rectBar, sc, barEndC);
      


      int hAlign, vAlign;
      attr.GetAlignment(&hAlign, &vAlign);

      int overflowCols = 0;

      if (attr.GetOverflow()) {
        int cols = grid.GetNumberCols();
        int best_width = GetBestSize(grid,attr,dc,row,col).GetWidth();
        int cell_rows, cell_cols;
        attr.GetSize( &cell_rows, &cell_cols ); // shouldn't get here if <= 0
        if ((best_width > rectCell.width) && (col < cols) && grid.GetTable()) {
          int i, c_cols, c_rows;
          for (i = col+cell_cols; i < cols; i++) {
            bool is_empty = true;
            for (int j=row; j < row + cell_rows; j++) {
              // check w/ anchor cell for multicell block
              grid.GetCellSize(j, i, &c_rows, &c_cols);
              if (c_rows > 0)
                c_rows = 0;
              if (!grid.GetTable()->IsEmptyCell(j + c_rows, i)) {
                is_empty = false;
                break;
              }
            }

            if (is_empty) {
              rect.width += grid.GetColSize(i);
            } else {
              i--;
              break;
            }

            if (rect.width >= best_width)
              break;
          }

          overflowCols = i - col - cell_cols + 1;
          if (overflowCols >= cols)
            overflowCols = cols - 1;
        }

        if (overflowCols > 0) {// redraw overflow cells w/ proper hilight

          hAlign = wxALIGN_LEFT; // if oveflowed then it's left aligned
          wxRect clip = rect;
          clip.x += rectCell.width;
          // draw each overflow cell individually
          int col_end = col + cell_cols + overflowCols;
          if (col_end >= grid.GetNumberCols())
            col_end = grid.GetNumberCols() - 1;
          for (int i = col + cell_cols; i <= col_end; i++) {
            clip.width = grid.GetColSize(i) - 1;
            dc.DestroyClippingRegion();
            dc.SetClippingRegion(clip);

            SetTextColoursAndFont(grid, attr, dc,
              grid.IsInSelection(row,i));

            grid.DrawTextRectangle(dc, grid.GetCellValue(row, col),
              rect, hAlign, vAlign);
            clip.x += grid.GetColSize(i) - 1;
          }

          rect = rectCell;
          rect.Inflate(-1);
          rect.width++;
          dc.DestroyClippingRegion();
        }
      }

      // now we only have to draw the text
      SetTextColoursAndFont(grid, attr, dc, false);

      grid.DrawTextRectangle(dc, grid.GetCellValue(row, col),
        rect, hAlign, vAlign);
  }
};

double ProfilerGridCellRenderer::m_maxValue = 0;

void StackWalkerMainWnd::ClearContext() {
  m_sourceEdit->SetReadOnly(false);
  m_sourceEdit->ClearAll();  
  m_sourceEdit->LoadFile("");
  m_sourceEdit->SetReadOnly(true);
  m_resultsGrid->SetTable(0);
  m_currentActiveFs = 0;
  m_callstackView->ShowCallstackToFunction("", true);
  m_currentFunction = "";
  m_currentSourceFile = "";
  UpdateTitleBar();
}

void StackWalkerMainWnd::RefreshGridView() {
  ClearContext();
  if (!g_displayedSampleInfo)
    return;
  if (!g_displayedSampleInfo->m_totalSamples)
    g_displayedSampleInfo->m_totalSamples = 1;
  // func, samples, file, lines, module

  int ignoredSamples = g_displayedSampleInfo->GetIgnoredSamples();

  m_resultsGrid->CreateGrid(g_displayedSampleInfo->m_sortedFunctionSamples.size(), 5);  
  m_resultsGrid->BeginBatch();
  m_resultsGrid->SetColLabelValue(0, "Function");
  m_resultsGrid->SetColLabelValue(1, "Samples");
  m_resultsGrid->SetColLabelValue(2, "Src File");
  m_resultsGrid->SetColLabelValue(3, "Lines");
  m_resultsGrid->SetColLabelValue(4, "Module");
  ProfilerGridCellRenderer::m_maxValue = 0;
  int n = g_displayedSampleInfo->m_sortedFunctionSamples.size() - 1;
  for (std::list<FunctionSample *> ::iterator it = g_displayedSampleInfo->m_sortedFunctionSamples.begin();
    it != g_displayedSampleInfo->m_sortedFunctionSamples.end(); ++it) {
      m_resultsGrid->SetCellValue(n, 0, m_settings.DoAbbreviations((*it)->m_functionName));
      m_resultsGrid->SetCellBackgroundColour(n, 0, m_resultsGrid->GetLabelBackgroundColour());
      char buf[256];
      if ((*it)->m_bIgnoredFromDisplay) {
        sprintf(buf, "ignored");
      } else {
        double val = (100.0 * (double)((*it)->m_sampleCount) / (g_displayedSampleInfo->m_totalSamples - ignoredSamples));
        if (m_bWievSamplesAsSampleCounts) {
          val = (*it)->m_sampleCount;
          sprintf(buf, "%0.0f", val);
        } else {
          sprintf(buf, "%0.1lf%%", val);
        }
        if (val > ProfilerGridCellRenderer::m_maxValue) {
          ProfilerGridCellRenderer::m_maxValue = val;
        }
      }
      m_resultsGrid->SetCellValue(n, 1, buf);
      m_resultsGrid->SetCellRenderer(n, 1, new ProfilerGridCellRenderer);
      m_resultsGrid->SetCellValue(n, 2, (*it)->m_fileName.c_str());
      sprintf(buf, "%d - %d", (*it)->m_minLine, (*it)->m_maxLine);
      m_resultsGrid->SetCellValue(n, 3, buf);
      m_resultsGrid->SetCellValue(n, 4, (*it)->m_moduleName.c_str());          
      n--;    
  }

  m_resultsGrid->AutoSizeColumns();  
  m_resultsGrid->AutoSizeRows();
  m_resultsGrid->SetColMinimalAcceptableWidth(10);
  m_resultsGrid->SetColMinimalWidth(0, 50);
  if (m_resultsGrid->GetColSize(0) > 500) {
    m_resultsGrid->SetColSize(0, 500);
  }
  m_resultsGrid->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
  m_resultsGrid->SetRowLabelSize(0);
  m_resultsGrid->SetColLabelSize(wxGRID_AUTOSIZE);  
  m_resultsGrid->EnableEditing(false);
  m_resultsGrid->DisableDragRowSize();
  m_resultsGrid->EnableDragColSize();
  m_resultsGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
  m_resultsGrid->SetCursorMode();
  m_resultsGrid->EndBatch();
  int size = 0;
  for (int i = 0; i < m_resultsGrid->GetNumberCols(); i++) {
    size += m_resultsGrid->GetColSize(i);  
  }
  m_gridWidth = size + 16;
  wxSize clientSz = GetClientSize();

  int pos = clientSz.GetX() / 2;

  wxSize clientSize = m_resultsGrid->GetClientSize();
  wxSize totalSize = m_resultsGrid->GetSize();    
  int extra = totalSize.GetX() - clientSize.GetX();  
  if (pos > m_gridWidth + extra)
    pos = m_gridWidth + extra;
  m_horzSplitter->SetSashPosition(pos);
}

void StackWalkerMainWnd::ThreadSelectionChanged() {
  for (int i = 0; i < m_toolbarThreadsListPopup->GetItemCount(); ++i) {
    bool bSelected = !!m_toolbarThreadsListPopup->GetItemState(i, wxLIST_STATE_SELECTED);
    unsigned int threadId = 0;
    wxString text = m_toolbarThreadsListPopup->GetItemText(i);
    sscanf(text.c_str(), " 0x%X", &threadId);
    for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin();
      it != g_threadSamples.end(); it++) {
        if (it->first == threadId) {
          it->second.m_bSelectedForDisplay = bSelected;
          break;
        }
    }
  }
  ProduceDisplayData();
  RefreshGridView();
}

void StackWalkerMainWnd::ProfileDataChanged() {

  m_toolbarThreadsListPopup->ClearAll();
  std::list<std::map<unsigned int, ThreadSampleInfo>::iterator> threadsSorted;  
  for (std::map<unsigned int, ThreadSampleInfo>::iterator it = g_threadSamples.begin(); it != g_threadSamples.end(); it++) {    
    bool bAdded = false;
    for (std::list<std::map<unsigned int, ThreadSampleInfo>::iterator>::iterator listpos = threadsSorted.begin(); listpos != threadsSorted.end(); ++listpos) {      
      if (it->second.GetCPUTime_ms() > (*listpos)->second.GetCPUTime_ms()) {
        threadsSorted.insert(listpos, it);
        bAdded = true;
        break;
      }
    }
    if (!bAdded) {
      threadsSorted.push_back(it);
    }    
  }

  for (std::list<std::map<unsigned int, ThreadSampleInfo>::iterator>::iterator listpos = threadsSorted.begin(); listpos != threadsSorted.end(); ++listpos) {  
    std::map<unsigned int, ThreadSampleInfo>::iterator it = *listpos;
    char buf[256];
    sprintf(buf, "0x%X [%d samples, %d funcs, %0.2lfs CPU time]", it->first, it->second.m_totalSamples, it->second.m_sortedFunctionSamples.size(), it->second.GetCPUTime_ms()/1000.0);
    m_toolbarThreadsListPopup->InsertItem(m_toolbarThreadsListPopup->GetItemCount(), buf);
  }
  
  for (int i = 1; i < m_toolbarThreadsListPopup->GetItemCount(); i++) {
    m_toolbarThreadsListPopup->Select(i, false);
  }
  if (m_toolbarThreadsListPopup->GetItemCount()) {
    m_toolbarThreadsListPopup->Select(0, true);  
    m_toolbarThreadsCombo->SetText(m_toolbarThreadsListPopup->GetItemText(0));
  } else {
    m_toolbarThreadsCombo->SetText("");
  }
  ThreadSelectionChanged();
  RefreshGridView();
}


void StackWalkerMainWnd::OnProfileRun(wxCommandEvent& WXUNUSED(event)) {  
  if (m_settings.m_executable.empty() && !m_settings.m_bAttachToProcess) {
    wxMessageDialog dlg(this, _("You need to set the name of the executable to be profiled first.\n"
                                "Use the 'Profile/Project setup...' menu command to do that."), _("Error"), wxOK|wxICON_ERROR);
    dlg.ShowModal();
    return;
  }
  
  if (ComplainAboutNonSavedProfile()) {
    return;
  }

  ShowChildWindows(true);  

  ClearContext();

  if (m_settings.m_currentDirectory.empty()) {
    wxFileName fn(m_settings.m_executable);
    m_settings.m_currentDirectory = fn.GetVolume() + fn.GetVolumeSeparator() + fn.GetPath(wxPATH_GET_SEPARATOR);
  }  
    
  m_logCtrl->Clear();
  m_currentSourceFile = "";
  m_sourceEdit->LoadFile("");
 
  if (!SampleProcessWithDialogProgress(this, &m_settings)) {
    wxMessageBox(wxT("Profiling failed."), wxT("Notification"), wxOK | wxCENTRE | wxICON_HAND);
  } else {
    g_bNewProfileData = true;
  }
  int w,h;
  m_logCtrl->GetSize(&w, &h);
  m_logCtrl->SetSize(w, h-1);
  m_logCtrl->SetSize(w, h);

  ProfileDataChanged(); 
}

void StackWalkerMainWnd::OnWizardFinished(wxWizardEvent& WXUNUSED(event)) {
  if (m_settingsBeforeWizard != m_settings) {
    m_settings.m_bChanged = TRUE;
  }  
  UpdateTitleBar();
}

void StackWalkerMainWnd::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
  //wxMessageBox(wxT("The wizard was cancelled."), wxT("Wizard notification"));
}

void StackWalkerMainWnd::OnZoomIn(wxCommandEvent& WXUNUSED(event)) {
  m_zoom *= 1.2;
  if (m_zoom > 2)
    m_zoom = 2;
  m_callstackView->SetZoom(m_zoom);
}

void StackWalkerMainWnd::OnZoomOut(wxCommandEvent& WXUNUSED(event)) {
  m_zoom /= 1.2;
  if (m_zoom < 0.1)
    m_zoom = 0.1;
  m_callstackView->SetZoom(m_zoom);
}

void StackWalkerMainWnd::OnMaximizeView(wxCommandEvent&) {

  if (m_verticalSplitterRestorePosition >= 0 &&
    m_horizontalSplitterRestorePosition >= 0) {
      RestoreViews();
      return;
  }

  wxWindow *pFocusWindow = wxWindow::FindFocus();
  m_verticalSplitterRestorePosition = m_vertSplitter->GetSashPosition();
  m_horizontalSplitterRestorePosition = m_horzSplitter->GetSashPosition();
  wxPoint sz = GetClientRect().GetBottomRight();  

  if (pFocusWindow == m_sourceEdit || pFocusWindow == m_sourceEdit->GetLineSampleView() ||
      pFocusWindow == m_sourceEdit->GetParent()) {
    m_horzSplitter->Unsplit(m_resultsGrid);
    m_vertSplitter->Unsplit(m_bottomNotebook);
  } else if (pFocusWindow == m_callstackView ||
    pFocusWindow == m_logCtrl ||
    pFocusWindow == m_bottomNotebook) {    
      m_vertSplitter->Unsplit(m_horzSplitter);
  } else {  
    // grid is in focus
    m_horzSplitter->Unsplit(m_editParent);
    m_vertSplitter->Unsplit(m_bottomNotebook);
  }

}

void StackWalkerMainWnd::RestoreViews() {
  if (m_verticalSplitterRestorePosition >= 0 &&
    m_horizontalSplitterRestorePosition >= 0) {
      m_vertSplitter->SplitHorizontally(m_horzSplitter, m_bottomNotebook, m_verticalSplitterRestorePosition);  
      m_horzSplitter->SplitVertically(m_resultsGrid, m_editParent, m_horizontalSplitterRestorePosition);
      m_verticalSplitterRestorePosition = -1;
      m_horizontalSplitterRestorePosition = -1;
  }
}


#include "abbreviationsDialog.h"

void StackWalkerMainWnd::OnViewAbbreviate(wxCommandEvent&) {
  std::map<wxString, wxString> before = m_settings.m_symbolAbbreviations;
  AbbreviationsDialog dlg(this, &m_settings.m_symbolAbbreviations);
  dlg.SetLongText(m_currentFunction);
  if (dlg.ShowModal() == wxID_OK) {
    if (before != m_settings.m_symbolAbbreviations) {
      m_settings.m_bChanged = TRUE;    
      RefreshGridView();
    }
  }
}

void StackWalkerMainWnd::OnViewIgnoreFunction(wxCommandEvent&) {
  if (m_currentActiveFs) {    
    m_currentActiveFs->m_bIgnoredFromDisplay = !m_currentActiveFs->m_bIgnoredFromDisplay;
    int row = m_resultsGrid->GetGridCursorRow();
    RefreshGridView();
    m_resultsGrid->SelectRow(row);  
    m_resultsGrid->SetGridCursor(row, 0);
    m_resultsGrid->SetFocus();
  }
}

void StackWalkerMainWnd::OnFileSaveProfile(wxCommandEvent&) {
  wxFileDialog fdlg(this, "Save profile as:",
    "", "", "Luke StackWalker data files (*.lsd)|*.lsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (fdlg.ShowModal() == wxID_CANCEL)
    return;
  if (!SaveSampleData(fdlg.GetPath().c_str())) {
    wxMessageBox(wxT("Saving the profile data failed."), wxT("Notification"));
  } else {
    m_fileHistory.AddFileToHistory(fdlg.GetPath());
    m_currentDataFile = fdlg.GetPath();
    UpdateTitleBar();
  }
}

void StackWalkerMainWnd::LoadProfileData(const char *fileName) {
  ShowChildWindows(true);
  ClearContext();
  if (!LoadSampleData(fileName)) {
    wxMessageBox(wxT("Loading the profile data failed."), wxT("Notification"));
    g_threadSamples.clear();
  } else {
    m_fileHistory.AddFileToHistory(fileName); 
    m_currentDataFile = fileName;
  }
  ProfileDataChanged();
}

void StackWalkerMainWnd::OnFileLoadProfile(wxCommandEvent&) {
  if (ComplainAboutNonSavedProfile()) {
    return;
  }
  ShowChildWindows(true);
  ClearContext();

  wxFileDialog fdlg(this, "Select a profile data file to load",
    "", "", "Luke StackWalker data files (*.lsd)|*.lsd");
  if (fdlg.ShowModal() == wxID_CANCEL)
    return;

  LoadProfileData(fdlg.GetPath().c_str());
  UpdateTitleBar();
}

void StackWalkerMainWnd::OnHorizontalSplitterChanging(wxSplitterEvent& event) {
  int pos = event.GetSashPosition();
  int extra = 0;
  wxSize clientSize = m_resultsGrid->GetClientSize();
  wxSize totalSize = m_resultsGrid->GetSize();    
  extra = totalSize.GetX() - clientSize.GetX();  
  if (pos > m_gridWidth + extra) {
    event.SetSashPosition(m_gridWidth + extra);
  }
}


void StackWalkerMainWnd::OnFindFunction(wxCommandEvent&) {
  if (!m_resultsGrid->wxWindow::IsShownOnScreen()) 
    return; 
  if ( m_pFindReplaceDialog ) {
    delete m_pFindReplaceDialog;
    m_pFindReplaceDialog = NULL;
  } else {
    m_pFindReplaceDialog = new wxFindReplaceDialog (
    this, m_pFindReplaceData, _T("Find a function in the profile"),
    wxFR_NOWHOLEWORD);
    m_pFindReplaceDialog->Show(true);
  }
}

bool FindMatch(wxString needle, wxString haystack, bool bCaseSensitive) {
  if (!bCaseSensitive) {
    needle = needle.MakeUpper();
    haystack = haystack.MakeUpper();
  }
  return haystack.Contains(needle);
}

void StackWalkerMainWnd::OnFindDialog(wxFindDialogEvent& event) {
  if (!m_resultsGrid->wxWindow::IsShownOnScreen()) 
    return;
  static bool bInMessageBox = false;
  if (bInMessageBox)
    return;
  
  wxEventType type = event.GetEventType();
  if ( type == wxEVT_COMMAND_FIND || type == wxEVT_COMMAND_FIND_NEXT ) {                             
    wxString findString = event.GetFindString().c_str();
    bool bDown = !!(event.GetFlags() & wxFR_DOWN);
    bool bCaseSensitive = !!(event.GetFlags() & wxFR_MATCHCASE);
    int curr = 0;
    wxArrayInt rows = m_resultsGrid->GetSelectedRows();
    if (rows.Count() && rows.Item(0)) {
      curr = rows.Item(0);
    }
    int step = 1;
    int end = m_resultsGrid->GetNumberRows();
    if (!bDown) {
      step = -1;
      end = -1;
    }
    
    for (curr = curr + step; curr != end; curr += step) {
      wxString str = m_resultsGrid->GetCellValue(curr, 0);
      if (FindMatch(findString, str, bCaseSensitive)) {
        m_resultsGrid->SelectRow(curr);
        m_resultsGrid->SetGridCursor(curr, 0);
        m_resultsGrid->MakeCellVisible(curr, 0);
        break;
      }
    }

    if (curr == end) {
      bInMessageBox = true;
      wxMessageBox("A function name containing the search string was not found.", wxT("Find Function"), wxOK | wxCENTRE);
      bInMessageBox = false;
    }
    
  }
}


void StackWalkerMainWnd::OnUpdateMenuItemSamplesAsPercentage(wxUpdateUIEvent& event) {
  event.Check(!m_bWievSamplesAsSampleCounts);
}

void StackWalkerMainWnd::OnUpdateMenuItemSamplesAsSampleCounts(wxUpdateUIEvent& event) {
  event.Check(m_bWievSamplesAsSampleCounts);
}

void StackWalkerMainWnd::OnViewSamplesAsSampleCounts(wxCommandEvent&) {
  if (m_bWievSamplesAsSampleCounts)
    return;
  m_bWievSamplesAsSampleCounts = true;
  m_sourceEdit->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
  m_callstackView->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
  RefreshGridView();
}

void StackWalkerMainWnd::OnViewSamplesAsPercentage(wxCommandEvent&) {
  if (!m_bWievSamplesAsSampleCounts)
    return;
  m_bWievSamplesAsSampleCounts = false;
  m_sourceEdit->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
  m_callstackView->SetShowSamplesAsSampleCounts(m_bWievSamplesAsSampleCounts);
  RefreshGridView();
}

void LogMessage(bool bError, const char *format, ...) {
  if (!s_pLogCtrl)
    return;
  wxTextAttr attr = s_pLogCtrl->GetDefaultStyle();
  if (bError) {
    attr.SetTextColour(*wxRED);
    s_pLogCtrl->SetDefaultStyle(attr);
  }

  char buffer[10240];
  va_list args;
  va_start (args, format);
  vsnprintf_s (buffer, sizeof(buffer), _TRUNCATE, format, args);
  perror (buffer);
  va_end (args);

  wxLogMessage(buffer);

  if (bError) {
    attr.SetTextColour(*wxBLACK);
    s_pLogCtrl->SetDefaultStyle(attr);
  }
  int pos = s_pLogCtrl->GetScrollRange(wxVERTICAL);
  s_pLogCtrl->SetScrollPos(wxVERTICAL, pos);
  s_pLogCtrl->ScrollLines(1);
}