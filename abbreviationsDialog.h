#pragma once

class AbbreviationsDialog : public wxDialog {
public:
  AbbreviationsDialog(wxWindow *parent, std::map<wxString, wxString> *abbreviationsMap);

  void OnAddButton(wxCommandEvent&);
  void OnOk(wxCommandEvent&);
  void OnRemoveButton(wxCommandEvent& );
  void OnItemSelected(wxCommandEvent& );
  void SetLongText(const char *txt) {m_longText->SetValue(txt);}

private:
  void CopyLocalMapToListCtrl();
  std::map<wxString, wxString> *m_abbreviationsMap;
  std::map<wxString, wxString> m_localMap;

  wxButton *m_addButton,
           *m_removeButton,
           *m_okButton,
           *m_cancelButton;

  wxTextCtrl *m_longText, *m_shortText;

  wxListBox *m_allItems;
  

  DECLARE_EVENT_TABLE()
};