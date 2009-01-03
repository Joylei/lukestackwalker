
#include <map>
#include <string>
#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/textCtrl.h>
#include <wx/listBox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "abbreviationsdialog.h"

enum {
  ID_ADD = wxID_HIGHEST + 1,
  ID_REMOVE, 
  ID_ALLITEMS_LB,
  ID_LONG_TEXT,
  ID_SHORT_TEXT
};

AbbreviationsDialog::AbbreviationsDialog(wxWindow *parent,   
                                         std::map<wxString, wxString> *abbreviationsMap) 
  : wxDialog(parent, wxID_ANY, wxString(_T("Symbol Abbreviations")))

{
  m_abbreviationsMap = abbreviationsMap;
  wxBoxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
  SetSizer(sizerTop);

  wxStaticText *static1 = new wxStaticText(this, wxID_ANY, "All abbreviations:");
  sizerTop->Add(static1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

  m_allItems = new wxListBox(this, ID_ALLITEMS_LB, wxDefaultPosition, wxSize(300, 100));
  sizerTop->Add(m_allItems, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

  {
    wxStaticText *static1 = new wxStaticText(this, wxID_ANY, "Partial Symbol:");
    sizerTop->Add(static1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
  }

  m_longText = new wxTextCtrl(this, ID_LONG_TEXT, "", wxDefaultPosition, wxSize(300, wxDefaultCoord));
  sizerTop->Add(m_longText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

  {
    wxStaticText *static1 = new wxStaticText(this, wxID_ANY, "Abbreviation:");
    sizerTop->Add(static1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
  }

  m_shortText = new wxTextCtrl(this, ID_SHORT_TEXT, "", wxDefaultPosition, wxSize(300, wxDefaultCoord));
  sizerTop->Add(m_shortText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );
  

  wxBoxSizer *sizerBottomRow = new wxBoxSizer(wxHORIZONTAL);      

  m_addButton = new wxButton(this, ID_ADD, _T("&Add/Modify"));  
  sizerBottomRow->Add(m_addButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
  m_removeButton = new wxButton(this, ID_REMOVE, _T("&Delete"));
  sizerBottomRow->Add(m_removeButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );


  m_okButton = new wxButton(this, wxID_OK, _T("&OK"));
  sizerBottomRow->Add(m_okButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
  m_cancelButton = new wxButton(this, wxID_CANCEL, _T("&Cancel"));
  sizerBottomRow->Add(m_cancelButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
  sizerTop->Add(sizerBottomRow, 0, wxTOP, 10);

  sizerTop->SetSizeHints(this);
  sizerTop->Fit(this);

  m_okButton->SetFocus();
  m_okButton->SetDefault();

  m_localMap = *m_abbreviationsMap;
  CopyLocalMapToListCtrl();
}

void AbbreviationsDialog::CopyLocalMapToListCtrl() {
  m_allItems->Freeze();
  m_allItems->Clear();
  for (std::map<wxString, wxString>::iterator it = m_localMap.begin(); it != m_localMap.end(); ++it) {
    wxString listItem = it->first + wxString(" --> ") + it->second;
    m_allItems->Append(listItem);
  }
  m_allItems->Thaw();
}

BEGIN_EVENT_TABLE(AbbreviationsDialog, wxDialog)
EVT_BUTTON(wxID_OK, AbbreviationsDialog::OnOk)
EVT_BUTTON(ID_ADD, AbbreviationsDialog::OnAddButton)
EVT_BUTTON(ID_REMOVE, AbbreviationsDialog::OnRemoveButton)
EVT_LISTBOX(ID_ALLITEMS_LB, OnItemSelected)  
END_EVENT_TABLE()

void AbbreviationsDialog::OnOk(wxCommandEvent& ev) {
  *m_abbreviationsMap = m_localMap;
  ev.Skip();
}

void AbbreviationsDialog::OnItemSelected(wxCommandEvent& ) {
  int sel = m_allItems->GetSelection();
  if (sel == wxNOT_FOUND) {
    return;
  }
  wxString shortT;
  wxString longT;

  int n = 0;
  for (std::map<wxString, wxString>::iterator it = m_localMap.begin(); it != m_localMap.end(); ++it) {
    if (n == sel) {
      longT = it->first;
      shortT = it->second;
      break;
    }
    n++;
  }
  m_longText->SetValue(longT);
  m_shortText->SetValue(shortT);
}

void AbbreviationsDialog::OnAddButton(wxCommandEvent&) {
  wxString longT = m_longText->GetValue();
  wxString shortT = m_shortText->GetValue();
  if (longT.length()) {
    m_localMap[longT] = shortT;
    CopyLocalMapToListCtrl();
  }
}

void AbbreviationsDialog::OnRemoveButton(wxCommandEvent&) {
  int sel = m_allItems->GetSelection();
  if (sel == wxNOT_FOUND) {
    return;
  }
  int n = 0;
  for (std::map<wxString, wxString>::iterator it = m_localMap.begin(); it != m_localMap.end(); ++it) {
    if (n == sel) {
      m_localMap.erase(it);
      break;
    }
    n++;
  }
  m_longText->SetValue("");
  m_shortText->SetValue("");
  CopyLocalMapToListCtrl();
}

