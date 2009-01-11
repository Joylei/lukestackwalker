#include <string>
#include <map>
#include <list>
#include <map>
#include <limits>
#include <vector>
#include "profilersettings.h"
#include "graphviz\include\gvc.h"

struct FunctionSample;


struct Caller;

struct Callee {
  Callee() {
    m_target = 0;
    m_count = 0;
    m_graphEdge = 0;
  }
  // call graph edge
  Caller *m_target;
  int m_count;
  Agedge_t *m_graphEdge;
};


struct Caller {
  // call graph node
  Caller() {
    m_functionSample = 0;
    m_sampleCount = 0;
    m_lineNumber = -1;
    m_graphNode = 0;
    m_ordinalForSaving = 0;
  }
  FunctionSample *m_functionSample;
  std::list<Callee>m_callees;
  int m_sampleCount;
  int m_lineNumber;  
  Agnode_t *m_graphNode;
  int m_ordinalForSaving;
};

struct LineInfo {
  LineInfo() {
    m_sampleCount = 0;
  }
  int m_sampleCount;
  //std::list<Caller> m_callers;
};

struct FileLineInfo {
  std::string m_fileName;
  std::vector<LineInfo> m_lineSamples;
};

#undef max
struct FunctionSample { // data struct for top-of-stack samples
  FunctionSample() {
    m_sampleCount = 0;
    m_maxLine = -1;
    m_minLine = std::numeric_limits<int>::max();
    m_bIgnoredFromDisplay = false;
  }
  std::string m_functionName;
  std::string m_fileName;
  std::string m_moduleName;

  int m_minLine;
  int m_maxLine;

  int m_sampleCount; // times the stack has been sampled while this function is on the top of the call stack
  std::list<Caller> m_callgraph;  // caller info for the whole function - only updated when function is at the top of stack
  bool m_bIgnoredFromDisplay;
};

struct ThreadSampleInfo {
  std::map<std::string, FunctionSample> m_functionSamples;
  std::map<std::string, FileLineInfo> m_lineSamples;
  std::list<FunctionSample *> m_sortedFunctionSamples;
  int m_totalSamples;
  bool m_bSelectedForDisplay;
  ThreadSampleInfo() {
    m_totalSamples = 0;
    m_bSelectedForDisplay = false;
  }
};



extern int g_allThreadSamples;
extern int g_totalModules;
extern int g_loadedModules;
extern bool g_bNewProfileData;

extern std::map<unsigned int, ThreadSampleInfo> g_threadSamples;

bool SampleProcess(ProfilerSettings *settings, ProfilerProgressStatus *status);

void SelectThreadForDisplay(unsigned int threadId, bool bSelect = true);
void ProduceDisplayData();

extern ThreadSampleInfo *g_displayedSampleInfo;

bool LoadSampleData(const wxString &fn);
bool SaveSampleData(const wxString &fn);