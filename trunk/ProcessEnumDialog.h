#pragma once

class wxCheckBox;
class wxListBox;
class ProcessEnumDialog : public wxDialog {
  void OnOk(wxCommandEvent&);
  void OnRefresh(wxCommandEvent&);
  wxListBox *m_items;
  void RefreshProcesses();

  wxButton *m_refreshButton,
           *m_okButton,
           *m_cancelButton;

  wxCheckBox *m_sortByNameCheckBox;


public:
  ProcessEnumDialog(wxWindow *parent);
  
  unsigned int m_processId;
  
  DECLARE_EVENT_TABLE()
};