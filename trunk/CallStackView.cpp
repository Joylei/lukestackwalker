#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "callstackview.h"
#include <wx/dcclient.h>
#include "sampledata.h"
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

BEGIN_EVENT_TABLE(CallStackView, wxScrolledWindow)
EVT_PAINT  (CallStackView::OnPaint)
EVT_LEFT_UP (CallStackView::OnLeftButtonUp)
END_EVENT_TABLE()

CallStackView::CallStackView(wxNotebook *parent, ProfilerSettings *pSettings) :
    wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    m_pSettings(pSettings)  
{
  m_parent = parent;
  m_bShowSamplesAsSampleCounts = true;
  wxFont m_font (8, wxMODERN, wxNORMAL, wxFONTWEIGHT_NORMAL, false, "Courier New");
  wxASSERT(m_font.IsOk());
  m_gvc = gvContext();
  m_fs = 0;
  m_zoom = 1.0; 
  m_graph = 0;
  enum {MAXWIDTH=2};
  enum {RSTART = 250, GSTART = 239, BSTART = 50, 
         REND = 250, GEND= 0, BEND = 0,
         RSTEP = (REND - RSTART)/NPENS,
         GSTEP = (GEND - GSTART)/NPENS,
         BSTEP = (BEND - BSTART)/NPENS};
  for (int i = 0 ; i < NPENS; i++) {
    int w = (MAXWIDTH*(i+i))/NPENS;
    m_pens[i] = new wxPen(wxColour(0, 0, 0), w?w:1, w?wxSOLID:wxDOT);
    m_brushes[i] = new wxBrush(wxColour(RSTART + RSTEP*i, GSTART + GSTEP*i, BSTART + BSTEP*i));
  }
  m_pcb = 0;
}

CallStackView::~CallStackView() {
  if (m_graph) {
    gvFreeLayout(m_gvc, m_graph);
    agclose(m_graph);
    gvFreeContext(m_gvc);
  }
  for (int i = 0 ; i < NPENS; i++) {
    delete m_pens[i];
    delete m_brushes[i];
  }
}


void CallStackView::DoGraph(FunctionSample *fs, bool bSkipPCInUnknownModules) {
  if (m_graph) {
    gvFreeLayout(m_gvc, m_graph);
    agclose(m_graph);
  }

  /* Create a simple digraph */
  m_graph = agopen("g", AGDIGRAPH);

  for (std::list<Caller>::iterator nodeit = fs->m_callgraph.begin(); 
    nodeit != fs->m_callgraph.end(); ++nodeit) {
      if (bSkipPCInUnknownModules && !nodeit->m_functionSample->m_moduleName.length())
        continue;
      char name[1024];
      
      int totalSamples = g_displayedSampleInfo->m_totalSamples - g_displayedSampleInfo->GetIgnoredSamples();

      if (nodeit == fs->m_callgraph.begin()) {
        char samples[64];
        if (m_bShowSamplesAsSampleCounts) {
          sprintf(samples, "%d", fs->m_sampleCount);
        } else {
          double val = (100.0 * (double)(fs->m_sampleCount) / totalSamples);
          sprintf(samples, "%0.1lf%%", val);          
        }
        _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\nsamples:%s", m_pSettings->DoAbbreviations(fs->m_functionName).c_str(), samples);
      } else {
        char samples[64];
        if (m_bShowSamplesAsSampleCounts) {
          sprintf(samples, "%d", nodeit->m_sampleCount);
        } else {
          double val = (100.0 * (double)(nodeit->m_sampleCount) / totalSamples);
          sprintf(samples, "%0.1lf%%", val);          
        }

        wxFileName fn(nodeit->m_functionSample->m_fileName); 
        if (!nodeit->m_functionSample->m_fileName.length()) {
         _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\n%s\nline:%d samples:%s", m_pSettings->DoAbbreviations(nodeit->m_functionSample->m_functionName).c_str(),
                  nodeit->m_functionSample->m_moduleName.c_str(), nodeit->m_lineNumber, samples);
        } else {
         _snprintf_s(name, sizeof(name), _TRUNCATE, "%s\n%s.%s\nline:%d samples:%s", m_pSettings->DoAbbreviations(nodeit->m_functionSample->m_functionName).c_str(),
                  fn.GetName().c_str(), fn.GetExt().c_str(), nodeit->m_lineNumber, samples);
        }
      }      
      nodeit->m_graphNode = agnode(m_graph, name);
      agsafeset(nodeit->m_graphNode, "shape", "box", "");      
  }
  for (std::list<Caller>::iterator nodeit = fs->m_callgraph.begin(); 
    nodeit != fs->m_callgraph.end(); ++nodeit) {
      if (bSkipPCInUnknownModules && !nodeit->m_functionSample->m_moduleName.length())
        continue;
      for (std::list<Call>::iterator edgeit = nodeit->m_callsFromHere.begin();
        edgeit != nodeit->m_callsFromHere.end(); ++edgeit) {
          if (edgeit->m_target->m_graphNode) {
            edgeit->m_graphEdge = agedge(m_graph, nodeit->m_graphNode, edgeit->m_target->m_graphNode);
          }
      }
  }

  /* Use the directed graph layout engine */
  gvLayout(m_gvc, m_graph, "dot");

  gvRender(m_gvc, m_graph, "dot", 0);  
}

void CallStackView::ShowCallstackToFunction(const char *funcName, bool bSkipPCInUnknownModules) {
  m_funcName = funcName;
  m_fs = 0;

  if (g_displayedSampleInfo) {
    std::map<std::string, FunctionSample>::iterator fsit = g_displayedSampleInfo->m_functionSamples.find(m_funcName);
    if (fsit != g_displayedSampleInfo->m_functionSamples.end()) {
      FunctionSample *fs = &fsit->second;
      DoGraph(fs, bSkipPCInUnknownModules);
      m_fs = fs;
    }
  }

  for (int i = 0; i < (int)m_parent->GetPageCount(); i++) {
    if (m_fs && m_parent->GetPage(i) == this) {
      m_parent->ChangeSelection(i);
      char buf[512];
      _snprintf_s(buf, sizeof(buf), _TRUNCATE, "Call Graph of %s", funcName);
      m_parent->SetPageText(i, buf);
      break;
    }
    if (!m_fs && m_parent->GetPage(i) == this) {
      m_parent->SetPageText(i, "Call Graph");
    }
    if (!m_fs && m_parent->GetPage(i) != this) {
      m_parent->ChangeSelection(i);      
    }
  }
  Scroll(0, 0);
  if (m_fs)
    Refresh();
}

static const  float xscale = 0.76f;

void CallStackView::OnDraw(wxDC &dc) {

  if (!m_fs) {
    return;
  }
  dc.SetUserScale(m_zoom, m_zoom);



  dc.SetFont(*wxNORMAL_FONT);

  wxSize windowSize = GetClientSize();
  windowSize.x = windowSize.x / m_zoom;
  windowSize.y = windowSize.y / m_zoom;
  wxSize maxPoint(m_graph->u.bb.UR.x*xscale, m_graph->u.bb.UR.y);
  if (maxPoint.x < windowSize.x)
    maxPoint.x = windowSize.x;
  if (maxPoint.y < windowSize.y)
    maxPoint.y = windowSize.y;
  dc.SetPen(*wxWHITE_PEN);
  dc.DrawRectangle(-1, -1, maxPoint.x + 30, maxPoint.y + 30);

  
  // draw graph edges
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      for (std::list<Call>::iterator edgeit = nodeit->m_callsFromHere.begin();
        edgeit != nodeit->m_callsFromHere.end(); ++edgeit) {
          int nPen = ((NPENS - 1) * edgeit->m_count) / (m_fs->m_sampleCount?m_fs->m_sampleCount:1);
          if (nPen >= NPENS) {
            nPen = NPENS - 1;
          }
          dc.SetPen(*m_pens[nPen]);            
          dc.SetBrush(*m_brushes[nPen]);
          if (edgeit->m_graphEdge && edgeit->m_graphEdge->u.spl) {
            Agedge_t *edge = edgeit->m_graphEdge;
            int n = edge->u.spl->list->size;
            wxPoint *pt = new wxPoint[n];
            for (int i = 0; i < n; i++) {
              pt[i].x = edge->u.spl->list->list[i].x*xscale;
              pt[i].y = edge->u.spl->list->list[i].y;
            }
            dc.DrawSpline(n,pt);
            if (edge->u.spl->list->eflag) {
              wxPoint ep(edge->u.spl->list->ep.x*xscale, edge->u.spl->list->ep.y);
              wxPoint sp = pt[n-1];
              wxPoint delta = sp-ep;
              int tmp = delta.x;
              delta.x = delta.y;
              delta.y = tmp;
              delta.x /= -2;
              delta.y /= 2;
              wxPoint arrow[3];
              arrow[0] = ep;
              arrow[1] = sp + delta;
              arrow[2] = sp - delta;
              dc.DrawPolygon(3, arrow);
            }
            delete [] pt;
          }
      }
  }

  // draw graph nodes
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      int nPen = ((NPENS - 1)* nodeit->m_sampleCount) / (m_fs->m_sampleCount?m_fs->m_sampleCount:1);
      if (nPen >= NPENS) {
        nPen = NPENS - 1;
      }
      dc.SetPen(*wxBLACK_PEN);
      dc.SetBrush(*m_brushes[nPen]);
      if (!nodeit->m_graphNode)
        continue;
      Agnode_t *node = nodeit->m_graphNode;
      dc.DrawRectangle(node->u.bb.LL.x*xscale, node->u.bb.LL.y,
        (node->u.bb.UR.x - node->u.bb.LL.x)*xscale,
        node->u.bb.UR.y - node->u.bb.LL.y);

      if (node->u.label && node->u.label->text) {
        wxString txt = node->u.label->text;
        int x = (node->u.bb.UR.x + node->u.bb.LL.x)*xscale / 2; // center of ellipse in x
        int y = node->u.label->p.y;
        do {           
          wxString str2 = txt.BeforeFirst('\n');
          wxSize sz = dc.GetTextExtent(str2);
          int yOffs = sz.y;
          if (nodeit != m_fs->m_callgraph.begin()) {
            yOffs += sz.y / 2;
          }
          dc.DrawText(str2, x - sz.x / 2, y -  yOffs);
          y += sz.y;
          txt = txt.AfterFirst('\n');
        } while (txt.length());
      }
  }


  SetVirtualSize(maxPoint.x * m_zoom, maxPoint.y * m_zoom);
  SetScrollRate(20, 20);
}

void CallStackView::OnLeftButtonUp(wxMouseEvent &evt) {
  wxClientDC dc(this);
  PrepareDC(dc);    
  dc.SetUserScale(m_zoom, m_zoom);
  wxPoint pt(evt.GetLogicalPosition(dc));
  if (!m_fs || !m_pcb) {
    return;
  }
  for (std::list<Caller>::iterator nodeit = m_fs->m_callgraph.begin(); 
    nodeit != m_fs->m_callgraph.end(); ++nodeit) {
      if (!nodeit->m_graphNode)
        continue;
      Agnode_t *node = nodeit->m_graphNode;
      wxRect nodeRect(node->u.bb.LL.x*xscale, node->u.bb.LL.y,
        (node->u.bb.UR.x - node->u.bb.LL.x)*xscale,
        node->u.bb.UR.y - node->u.bb.LL.y);
      if (nodeRect.Contains(pt)) {
        m_pcb->OnClickCaller(&(*nodeit));
      }
  }
}

void CallStackView::SetZoom(double zoom) {
  m_zoom = zoom;
  Refresh();
}
