//////////////////////////////////////////////////////////////////////////////
// File:        edit.h
// Purpose:     STC test module
// Maintainer:  Wyo
// Created:     2003-09-01
// RCS-ID:      $Id: edit.h 35523 2005-09-16 18:25:44Z ABX $
// Copyright:   (c) wxGuide
// Licence:     wxWindows licence
//////////////////////////////////////////////////////////////////////////////

#ifndef _EDIT_H_
#define _EDIT_H_

//----------------------------------------------------------------------------
// informations
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

//! wxWidgets headers

//! wxWidgets/contrib headers
#include "wx/stc/stc.h"  // styled text control

#include "prefs.h"

//============================================================================
// declarations
//============================================================================


class EditProperties;
class Edit;


wxColour GetGradientEndColorByFraction(const wxColour &startC, const wxColour &endC, double frac);


class LineSampleView : public wxWindow {
  Edit *m_pEdit;
  bool m_bShowSamplesAsSampleCounts;
public:
  void SetShowSamplesAsSampleCounts(bool bShowSampleCounts) {m_bShowSamplesAsSampleCounts = bShowSampleCounts; Refresh();}
	LineSampleView(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);	
	void OnPaint(wxPaintEvent& event);
  void SetEdit(Edit *pe) {m_pEdit = pe;}
protected:
	DECLARE_EVENT_TABLE()
  void OnDraw(wxPaintDC &dc);  
};

class Edit;

class EditParent : public wxWindow {
public:
	EditParent(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr);
	void OnSize(wxSizeEvent& event);
  void OnPaint(wxPaintEvent& event);
  Edit *GetEdit() {return m_edit;}
  
protected:
	DECLARE_EVENT_TABLE()
	LineSampleView *m_lineSampleView;
  wxFont m_font;
	Edit *m_edit;
};




//----------------------------------------------------------------------------
//! Edit
class Edit: public wxStyledTextCtrl {
    friend class EditProperties;
    LineSampleView *m_pLineSampleView;
public:
    //! constructor
    Edit (wxWindow *parent, wxWindowID id = wxID_ANY,
          const wxPoint &pos = wxDefaultPosition,
          const wxSize &size = wxDefaultSize,
          long style = wxSUNKEN_BORDER|wxVSCROLL
         );

    //! destructor
    ~Edit ();

    void ShowLineNumbers ();
    void SetShowSamplesAsSampleCounts(bool bShowSampleCounts) {m_pLineSampleView->SetShowSamplesAsSampleCounts(bShowSampleCounts);}

    // event handlers
    // common
    void OnSize( wxSizeEvent &event );
    // edit

    //! language/lexer
    wxString DeterminePrefs (const wxString &filename);
    bool InitializePrefs (const wxString &filename);
    bool UserSettings (const wxString &filename);
    LanguageInfo const* GetLanguageInfo () {return m_language;};

    //! load/save file
    bool LoadFile ();
    bool LoadFile (wxString filename, wxString openFrom = "");
    wxString GetFilename () {return m_filename;};
    void SetFilename (const wxString &filename) {m_filename = filename;};
    void OnPainted(wxStyledTextEvent &);
    void SetLineSampleView (LineSampleView *pv) {m_pLineSampleView = pv;}
    LineSampleView *GetLineSampleView() {return m_pLineSampleView;}

private:
    // file
    wxString m_filename;

    // lanugage properties
    LanguageInfo const* m_language;

    // margin variables
    int m_LineNrID;
    int m_LineNrMargin;
    int m_FoldingID;
    int m_FoldingMargin;
    int m_DividerID;

    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
//! EditProperties
class EditProperties: public wxDialog {

public:

    //! constructor
    EditProperties (Edit *edit, long style = 0);

private:

};



#endif // _EDIT_H_
