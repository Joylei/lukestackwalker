#pragma once
#include <wx/scrolwin.h>
#include <string>
#include "wx/notebook.h"
#include "graphviz\include\gvc.h"


struct FunctionSample;
struct Caller;
class wxBitmap;
class wxPen;
class ProfilerSettings;


class CallStackViewClickCallback {
public:
  virtual void OnClickCaller(Caller *caller) = 0;
};

class CallStackView: public wxScrolledWindow  {
  std::string m_funcName;
  wxNotebook *m_parent;
  wxFont m_font;
  GVC_t *m_gvc;
  FunctionSample *m_fs;
  Agraph_t *m_graph;
  double m_zoom;
  enum {NPENS = 20};
  wxPen *m_pens[NPENS];
  wxBrush *m_brushes[NPENS];
  CallStackViewClickCallback *m_pcb;
  ProfilerSettings *m_pSettings;
  bool m_bShowSamplesAsSampleCounts;
public:
  CallStackView( wxNotebook *parent, ProfilerSettings *pSettings );
  ~CallStackView();
  void ShowCallstackToFunction(const char *funcName, bool bSkipPCInUnknownModules);
  void DoGraph(FunctionSample *fs, bool bSkipPCInUnknownModules);
  
  void OnDraw(wxDC &dc);
  void OnLeftButtonUp(wxMouseEvent &evt);
  void SetZoom(double zoom);
  void SetClickCallback(CallStackViewClickCallback *pcb) {m_pcb = pcb;}
  void SetShowSamplesAsSampleCounts(bool bShowSampleCounts) {m_bShowSamplesAsSampleCounts = bShowSampleCounts;}
  DECLARE_EVENT_TABLE()
};