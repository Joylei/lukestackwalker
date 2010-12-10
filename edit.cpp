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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all 'standard' wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

//! wxWidgets headers
#include "wx/file.h"     // raw file io support
#include "wx/filename.h" // filename support
#include "wx/settings.h"

//! application headers

#include "edit.h"        // edit module
#include "sampledata.h"

//----------------------------------------------------------------------------
// LineSampleView
//----------------------------------------------------------------------------

static FileLineInfo *s_pfli = 0;
int s_maxSamplesPerLine = 0;

enum {LINESAMPLEWIDTH = 58, EDITHEADERHEIGHT=19};

BEGIN_EVENT_TABLE (LineSampleView, wxWindow)    
    EVT_PAINT(LineSampleView::OnPaint)
END_EVENT_TABLE()

LineSampleView::LineSampleView(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
	  wxWindow(parent, id, pos, size, style, name) {
   m_bShowSamplesAsSampleCounts = true;
};

wxColour GetGradientEndColorByFraction(const wxColour &sc, const wxColour &ec, double frac) {
   return wxColor ((unsigned char)(sc.Red() + frac * (ec.Red() - sc.Red())), 
                   (unsigned char)(sc.Green() + frac * (ec.Green() - sc.Green())), 
                   (unsigned char)(sc.Blue() + frac * (ec.Blue() - sc.Blue())));
}

void LineSampleView::OnDraw(wxPaintDC &dc) {
  wxSize sz = m_pEdit->GetClientSize();  
  
  int x = 0;
  int y = 0;

  
  wxColour bkc = GetBackgroundColour();
  wxBrush bkBrush(bkc);  
  
  wxPen   bkPen(bkc);
  dc.SetPen(*wxMEDIUM_GREY_PEN);
  dc.DrawLine(0, 0, LINESAMPLEWIDTH, 0);

  dc.DrawLine(0, 0, 0, sz.y+30);
  dc.SetPen(*wxGREY_PEN);
  dc.DrawLine(1, 1, LINESAMPLEWIDTH, 1);
  dc.DrawLine(1, 1, 1, sz.y+30);

  dc.SetPen(*wxLIGHT_GREY_PEN);
  int bottom = GetClientSize().y - 1;
  dc.DrawLine(1, bottom, LINESAMPLEWIDTH, bottom);

  m_pEdit->ClientToScreen(&x, &y);  
  this->ScreenToClient(&x, &y);  
  dc.SetPen(bkPen);
  dc.SetBrush(bkBrush);
  dc.SetFont(*wxNORMAL_FONT);
  
  if (!s_pfli || !g_displayedSampleInfo) {
    dc.DrawRectangle(2, y, LINESAMPLEWIDTH, y+sz.y);
    return;
  }
  int totalSamples = g_displayedSampleInfo->m_totalSamples - g_displayedSampleInfo->GetIgnoredSamples();  
  int startLine = m_pEdit->GetFirstVisibleLine();
  int lh = m_pEdit->TextHeight(startLine);
  for (int l = startLine; y < sz.y; l++) {
    char buf[256];
    wxRect rectBase(2, y, LINESAMPLEWIDTH, lh);
    dc.SetBrush(bkBrush);
    dc.DrawRectangle(rectBase);
    if (((int)s_pfli->m_lineSamples.size() > l+1)) {
      if(s_pfli->m_lineSamples[l+1].m_sampleCount) {
        wxRect rectBar(4, y+1, LINESAMPLEWIDTH, lh-4);
        double perc = (double)s_pfli->m_lineSamples[l+1].m_sampleCount / (s_maxSamplesPerLine?s_maxSamplesPerLine:1);
        rectBar.width = (int)(rectBar.width * perc);

        wxColor ec = *wxRED;
        wxColor sc = *wxGREEN;
        wxColour barEndC = GetGradientEndColorByFraction(sc, ec, perc);                      
        dc.GradientFillLinear(rectBar, sc, barEndC);
        if (m_bShowSamplesAsSampleCounts) {                    
          sprintf(buf, "%d", s_pfli->m_lineSamples[l+1].m_sampleCount);
        } else {
          sprintf(buf, "%0.2lf%%", (100.0 * s_pfli->m_lineSamples[l+1].m_sampleCount)/(totalSamples?totalSamples:1));
        }
        dc.DrawText(buf, 5, y);
      }
    }
    y+= m_pEdit->TextHeight(l);
  }  
}

void LineSampleView::OnPaint(wxPaintEvent&) {
  wxPaintDC dc(this);
  OnDraw(dc);
}

//----------------------------------------------------------------------------
// EditParent
//----------------------------------------------------------------------------


BEGIN_EVENT_TABLE (EditParent, wxWindow)    
    EVT_SIZE(EditParent::OnSize)
    EVT_PAINT(EditParent::OnPaint)    
END_EVENT_TABLE()

EditParent::EditParent(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
                       const wxSize& size, long style, const wxString& name) :
	wxWindow(parent, id, pos, size, style, name) {

  m_edit = new Edit(this);
  m_lineSampleView = new LineSampleView(this, wxID_ANY);
  m_edit->SetLineSampleView(m_lineSampleView);
  m_lineSampleView->SetEdit(m_edit);
  m_font = *wxNORMAL_FONT;
  m_font.SetWeight(wxBOLD);
};
  
void EditParent::OnPaint(wxPaintEvent&) {
  wxPaintDC dc(this);
  dc.SetFont(m_font);
  wxSize sz = GetClientSize();  
  dc.SetPen(*wxLIGHT_GREY_PEN);
  dc.DrawLine(0, 0, 0, EDITHEADERHEIGHT);
  dc.DrawLine(LINESAMPLEWIDTH+2, 0, LINESAMPLEWIDTH+2, EDITHEADERHEIGHT);

  wxString str = "Samples";
  wxSize txsz = dc.GetTextExtent(str);
  dc.DrawText(str, LINESAMPLEWIDTH/2-txsz.x/2, EDITHEADERHEIGHT/2-txsz.y/2);

  wxString fn = m_edit->GetFilename();
  bool bRemovedChars = false;
  int maxWidth = sz.x - LINESAMPLEWIDTH - 30;
  do {
    txsz = dc.GetTextExtent(fn);
    if (txsz.GetX() > maxWidth) {
      fn = fn.Right(fn.Length() - 1);
      bRemovedChars = true;
    }
  } while (fn.Length() && (txsz.GetX() > maxWidth));

  if (bRemovedChars) {
    for (int i = 0; i < 3 && i < (int)fn.Length(); i++) {
      fn.at(i) = '.';
    }
  }    
  dc.DrawText(fn.c_str(), LINESAMPLEWIDTH+3 + (sz.x - (LINESAMPLEWIDTH+3))/2 - txsz.x/2, EDITHEADERHEIGHT/2-txsz.y/2);
}

void EditParent::OnSize(wxSizeEvent &) {
  wxSize sz = GetClientSize();  
  m_lineSampleView->SetSize(0, EDITHEADERHEIGHT, LINESAMPLEWIDTH, sz.y-EDITHEADERHEIGHT);
  m_edit->SetSize(LINESAMPLEWIDTH+2, EDITHEADERHEIGHT, sz.x-LINESAMPLEWIDTH, sz.y-EDITHEADERHEIGHT);
  Refresh();
}


//----------------------------------------------------------------------------
// Edit
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (Edit, wxStyledTextCtrl)
    // common
    EVT_SIZE (                         Edit::OnSize)    
    EVT_STC_PAINTED(wxID_ANY, Edit::OnPainted)
    // stc
END_EVENT_TABLE()



void Edit::OnPainted(wxStyledTextEvent &) {
  m_pLineSampleView->Refresh();
}

Edit::Edit (wxWindow *parent, wxWindowID id,
            const wxPoint &pos,
            const wxSize &size,
            long style)
    : wxStyledTextCtrl (parent, id, pos, size, style) {

    m_filename = wxEmptyString;

    m_LineNrID = 0;
    m_DividerID = 1;
    m_FoldingID = 2;

    // initialize language
    m_language = NULL;

    // default font for all styles
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable?
                 wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (g_CommonPrefs.readOnlyInitial);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSTC_WRAP_WORD: wxSTC_WRAP_NONE);
    wxFont font (10, wxMODERN, wxNORMAL, wxNORMAL);
    StyleSetFont (wxSTC_STYLE_DEFAULT, font);
    StyleSetForeground (wxSTC_STYLE_DEFAULT, *wxBLACK);
    StyleSetBackground (wxSTC_STYLE_DEFAULT, *wxWHITE);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxWHITE);
    StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));
    InitializePrefs (DEFAULT_LANGUAGE);

    // set visibility
    SetVisiblePolicy (wxSTC_VISIBLE_STRICT|wxSTC_VISIBLE_SLOP, 1);
    SetXCaretPolicy (wxSTC_CARET_EVEN|wxSTC_VISIBLE_STRICT|wxSTC_CARET_SLOP, 1);
    SetYCaretPolicy (wxSTC_CARET_EVEN|wxSTC_VISIBLE_STRICT|wxSTC_CARET_SLOP, 1);

    // markers
    MarkerDefine (wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_DOTDOTDOT, _T("BLACK"), _T("BLACK"));
    MarkerDefine (wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_ARROWDOWN, _T("BLACK"), _T("BLACK"));
    MarkerDefine (wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_EMPTY,     _T("BLACK"), _T("BLACK"));
    MarkerDefine (wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_DOTDOTDOT, _T("BLACK"), _T("WHITE"));
    MarkerDefine (wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN, _T("BLACK"), _T("WHITE"));
    MarkerDefine (wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY,     _T("BLACK"), _T("BLACK"));
    MarkerDefine (wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_EMPTY,     _T("BLACK"), _T("BLACK"));

    // miscelaneous
    m_LineNrMargin = TextWidth (wxSTC_STYLE_LINENUMBER, _T("_999999"));
    m_FoldingMargin = 16;
    CmdKeyClear (wxSTC_KEY_TAB, 0); // this is done by the menu accelerator key
    SetLayoutCache (wxSTC_CACHE_PAGE);

}

Edit::~Edit () {}

//----------------------------------------------------------------------------
// common event handlers
void Edit::OnSize( wxSizeEvent& event ) {
    int x = GetClientSize().x +
            (g_CommonPrefs.lineNumberEnable? m_LineNrMargin: 0) +
            (g_CommonPrefs.foldEnable? m_FoldingMargin: 0);
    if (x > 0) SetScrollWidth (x);
    event.Skip();
}





void Edit::ShowLineNumbers() {
    SetMarginWidth (m_LineNrID, m_LineNrMargin);
}


//----------------------------------------------------------------------------
// private functions
wxString Edit::DeterminePrefs (const wxString &filename) {

    LanguageInfo const* curInfo;

    // determine language from filepatterns
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        wxString filepattern = curInfo->filepattern;
        filepattern.Lower();
        while (!filepattern.empty()) {
            wxString cur = filepattern.BeforeFirst (';');
            if ((cur == filename) ||
                (cur == (filename.BeforeLast ('.') + _T(".*"))) ||
                (cur == (_T("*.") + filename.AfterLast ('.')))) {
                return curInfo->name;
            }
            filepattern = filepattern.AfterFirst (';');
        }
    }
    return wxEmptyString;

}

bool Edit::InitializePrefs (const wxString &name) {

    // initialize styles
    StyleClearAll();
    LanguageInfo const* curInfo = NULL;

    // determine language
    bool found = false;
    int languageNr;
    for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++) {
        curInfo = &g_LanguagePrefs [languageNr];
        if (curInfo->name == name) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    // set lexer and language
    SetLexer (curInfo->lexer);
    m_language = curInfo;

    // set margin for line numbers
    SetMarginType (m_LineNrID, wxSTC_MARGIN_NUMBER);
    StyleSetForeground (wxSTC_STYLE_LINENUMBER, wxColour (_T("DARK GREY")));
    StyleSetBackground (wxSTC_STYLE_LINENUMBER, *wxWHITE);

    SetMarginWidth (m_LineNrID, 0);

    // default fonts for all styles!
    int Nr;
    for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++) {
        wxFont font (10, wxMODERN, wxNORMAL, wxNORMAL);
        StyleSetFont (Nr, font);
    }

    // set common styles
    StyleSetForeground (wxSTC_STYLE_DEFAULT, wxColour (_T("DARK GREY")));
    StyleSetForeground (wxSTC_STYLE_INDENTGUIDE, wxColour (_T("DARK GREY")));

    // initialize settings
    if (g_CommonPrefs.syntaxEnable) {
        int keywordnr = 0;
        for (Nr = 0; Nr < STYLE_TYPES_COUNT; Nr++) {
            if (curInfo->styles[Nr].type == -1) continue;
            const StyleInfo &curType = g_StylePrefs [curInfo->styles[Nr].type];
            wxFont font (curType.fontsize, wxMODERN, wxNORMAL, wxNORMAL, false,
                         curType.fontname);
            StyleSetFont (Nr, font);
            if (curType.foreground) {
                StyleSetForeground (Nr, wxColour (curType.foreground));
            }
            if (curType.background) {
                StyleSetBackground (Nr, wxColour (curType.background));
            }
            StyleSetBold (Nr, (curType.fontstyle & mySTC_STYLE_BOLD) > 0);
            StyleSetItalic (Nr, (curType.fontstyle & mySTC_STYLE_ITALIC) > 0);
            StyleSetUnderline (Nr, (curType.fontstyle & mySTC_STYLE_UNDERL) > 0);
            StyleSetVisible (Nr, (curType.fontstyle & mySTC_STYLE_HIDDEN) == 0);
            StyleSetCase (Nr, curType.lettercase);
            const wxChar *pwords = curInfo->styles[Nr].words;
            if (pwords) {
                SetKeyWords (keywordnr, pwords);
                keywordnr += 1;
            }
        }
    }

    // set margin as unused
    SetMarginType (m_DividerID, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth (m_DividerID, 0);
    SetMarginSensitive (m_DividerID, false);

    // folding
    SetMarginType (m_FoldingID, wxSTC_MARGIN_SYMBOL);
    SetMarginMask (m_FoldingID, wxSTC_MASK_FOLDERS);
    StyleSetBackground (m_FoldingID, *wxWHITE);
    SetMarginWidth (m_FoldingID, 0);
    SetMarginSensitive (m_FoldingID, false);

    // set spaces and indention
    SetTabWidth (4);
    SetUseTabs (false);
    SetTabIndents (true);
    SetBackSpaceUnIndents (true);
    SetIndent (g_CommonPrefs.indentEnable? 4: 0);

    // others
    SetViewEOL (g_CommonPrefs.displayEOLEnable);
    SetIndentationGuides (g_CommonPrefs.indentGuideEnable);
    SetEdgeColumn (80);
    SetEdgeMode (g_CommonPrefs.longLineOnEnable? wxSTC_EDGE_LINE: wxSTC_EDGE_NONE);
    SetViewWhiteSpace (g_CommonPrefs.whiteSpaceEnable?
                       wxSTC_WS_VISIBLEALWAYS: wxSTC_WS_INVISIBLE);
    SetOvertype (g_CommonPrefs.overTypeInitial);
    SetReadOnly (true);
    SetWrapMode (g_CommonPrefs.wrapModeInitial?
                 wxSTC_WRAP_WORD: wxSTC_WRAP_NONE);

    return true;
}

bool Edit::LoadFile ()
{
#if wxUSE_FILEDLG
    // get filname
    if (!m_filename) {
        wxFileDialog dlg (this, _T("Open file"), wxEmptyString, wxEmptyString,
                          _T("Any file (*)|*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
        if (dlg.ShowModal() != wxID_OK) return false;
        m_filename = dlg.GetPath();
    }

    // load file
    return LoadFile (m_filename);
#else
    return false;
#endif // wxUSE_FILEDLG
}

bool Edit::LoadFile (wxString filename, wxString openFrom) {

  // load file in edit and clear undo
  m_filename = filename;
  
  Freeze();
  
  if (g_displayedSampleInfo) {
    std::map<std::string, FileLineInfo>::iterator it =  g_displayedSampleInfo->m_lineSamples.find(filename.c_str());  
    if (it != g_displayedSampleInfo->m_lineSamples.end()) {
      s_pfli = &it->second;
    }
  } else {
    s_pfli = 0;
  }

  ClearAll ();
  EmptyUndoBuffer();

  
  if (openFrom.length()) {
    filename = openFrom;
  }
  wxFile file;
  if (wxFile::Exists(filename)) {
    file.Open(filename);
  }
  if (!file.IsOpened()) {
    s_pfli = 0;
    m_LineNrMargin = 0;
    ShowLineNumbers();
    Thaw();
    if (!m_filename.length()) {
      GetParent()->Refresh();
      return false;
    }
    wxString errMsg = "File ";
    errMsg += m_filename;
    errMsg += " could not be opened.\nUse the menu command File/Load Source File to open it.";
    InsertText (GetTextLength(), errMsg);
    m_filename = "";
    GetParent()->Refresh();
    return false;
  }

  s_maxSamplesPerLine = 0;
  for (int i = 0; i < (int)s_pfli->m_lineSamples.size(); i++) {
    if (s_pfli->m_lineSamples[i].m_sampleCount > s_maxSamplesPerLine)
      s_maxSamplesPerLine = s_pfli->m_lineSamples[i].m_sampleCount;
  }
  

  int line = 1;
  long lng = file.Length ();
  if (lng > 0) {
    wxString buf;
    wxChar *buff = buf.GetWriteBuf (lng);
    file.Read (buff, lng);
    buf.UngetWriteBuf ();    
    while (buf.Len()) {
      wxString lns = buf.BeforeFirst('\n');
      int right = buf.Len() - lns.Len() - 1;
      if (right < 0)
        right = 0;
      buf = buf.Right(right);    
      InsertText (GetTextLength(), lns);
      line++;
    }
  }
  file.Close();

  wxString margin = "__";
  while (line / 10) {
    margin += "9";
    line /= 10;
  }

  
  m_LineNrMargin = TextWidth (wxSTC_STYLE_LINENUMBER, margin);
  ShowLineNumbers();
  

  wxFileName fname (m_filename);
  InitializePrefs (DeterminePrefs (fname.GetFullName()));

  Thaw();
  GetParent()->Refresh();

  return true;
}

