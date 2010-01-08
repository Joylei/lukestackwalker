#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "profilersettings.h"

#define FILETYPEID "Luke StackWalker Project file version %d"

bool ProfilerSettings::SaveAs(std::string fname) {
  FILE *f = fopen(fname.c_str(), "w");
  if (!f)
    return false;
  fprintf(f, FILETYPEID"\n", CURRENT_VERSION);
  fprintf(f, "exe=%s\n", m_executable.c_str());
  fprintf(f, "args=%s\n", m_commandLineArgs.c_str());
  fprintf(f, "ndebugPaths=%d\n", m_debugInfoPaths.size());
  for (std::list<wxString>::iterator it = m_debugInfoPaths.begin();
       it != m_debugInfoPaths.end(); ++it) {
         fprintf(f, "debugpath=%s\n", it->c_str());    
  }
  fprintf(f, "sampledepth=%d\n", m_sampleDepth);
  fprintf(f, "samplingstartdelay=%d\n", m_samplingStartDelay);
  fprintf(f, "samplingtime=%d\n", m_samplingTime);
  fprintf(f, "currentdirectory=%s\n", m_currentDirectory.c_str());
  fprintf(f, "source substitutions=%d\n", m_sourceFileSubstitutions.size());
  for (std::map<wxString, wxString>::iterator it = m_sourceFileSubstitutions.begin(); 
       it != m_sourceFileSubstitutions.end(); ++it) {
    fprintf(f, "orig.file=%s\n", it->first.c_str());
    fprintf(f, "used file=%s\n", it->second.c_str());
  }

  fprintf(f, "symbol abbreviations=%d\n", m_symbolAbbreviations.size());
  for (std::map<wxString, wxString>::iterator it = m_symbolAbbreviations.begin(); 
       it != m_symbolAbbreviations.end(); ++it) {
    fprintf(f, "partial symbol=%s\n", it->first.c_str());
    fprintf(f, "substitute=%s\n", it->second.c_str());
  }
  fprintf(f, "useSymbolServer=%d\n", (int)m_bConnectToSymServer);

  fprintf(f, "environment variables=%d\n", m_environmentVariables.size());
  for (std::map<wxString, wxString>::iterator it = m_environmentVariables.begin(); 
       it != m_environmentVariables.end(); ++it) {
    fprintf(f, "variable=%s\n", it->first.c_str());
    fprintf(f, "value=%s\n", it->second.c_str());
  }
  fprintf(f, "attach to process=%d\n", m_bAttachToProcess);

  fprintf(f, "abort stack walk if address outside known modules=%d\n", m_bStopAtPCOutsideModules);

  m_bChanged = false;
  return (fclose(f) == 0);
}

bool ProfilerSettings::Load(std::string fname) {
  FILE *f = fopen(fname.c_str(), "r");
  if (!f)
    return false;
  m_sourceFileSubstitutions.clear();
#define ERR {fclose(f); return false;}
  int fileVer = 0;
  if (fscanf(f, FILETYPEID, &fileVer) != 1) {
    ERR;
  }
  char buf[1024];
  
  buf[0] = 0;
  fscanf(f, " exe =%1023[^\n]", buf);
  m_executable = buf;
  buf[0] = 0;
  fscanf(f, " args =%1023[^\n]", buf);  
  m_commandLineArgs = buf;

  int nPaths = 0;
  if (fscanf(f, " ndebugPaths = %d", &nPaths) != 1) {
    ERR;
  }
  m_debugInfoPaths.clear();
  for (int i = 0; i < nPaths; i++) {
    if (fscanf(f, " debugpath =%1023[^\n]", buf) != 1) {
      ERR;
    }
    m_debugInfoPaths.push_back(buf);
  }
  if (fscanf(f, " sampledepth =%d ", &m_sampleDepth) != 1) {
    ERR;
  }
  if (fscanf(f, " samplingstartdelay =%d ", &m_samplingStartDelay) != 1) {
    ERR;
  }
  if (fscanf(f, " samplingtime =%d ", &m_samplingTime) != 1) {
    ERR;
  }
  if (fileVer >= 2) {
    buf[0] = 0;
    fscanf(f, " currentdirectory =%1023[^\n]", buf);
    m_currentDirectory = buf;
  }
  if (fileVer >= 3) {
    int nSubs = 0;
    if (fscanf(f, " source substitutions =%d ", &nSubs) != 1) {
      ERR;
    }
    for (int i = 0; i < nSubs; i++) {
      char orig[2048] = {0};
      char subs[2048] = {0};
      fscanf(f, " orig.file =%1023[^\n]", orig);
      fscanf(f, " used file =%1023[^\n]", subs);
      m_sourceFileSubstitutions[orig] = subs;
    }
  }
  if (fileVer >= 4) {
    int nSubs = 0;
    if (fscanf(f, " symbol abbreviations =%d ", &nSubs) != 1) {
      ERR;
    }
    for (int i = 0; i < nSubs; i++) {
      char orig[2048] = {0};
      char subs[2048] = {0};
      fscanf(f, " partial symbol =%1023[^\n]", orig);
      fscanf(f, " substitute =%1023[^\n]", subs);
      m_symbolAbbreviations[orig] = subs;
    }
  }
  if (fileVer >= 5) {
    int dummy = 1;
    if (fscanf(f, " useSymbolServer = %d\n", &dummy) != 1) {
      ERR;
    }
    m_bConnectToSymServer = !!dummy;
  }
  if (fileVer >= 6) {
    int nVars = 0;
    if (fscanf(f, " environment variables =%d ", &nVars) != 1) {
      ERR;
    }
    for (int i = 0; i < nVars; i++) {
      char var[2048] = {0};
      char val[2048] = {0};
      fscanf(f, " variable =%1023[^\n]", var);
      fscanf(f, " value =%1023[^\n]", val);
      m_environmentVariables[var] = val;
    }
  }
  if (fileVer >= 7) {
    int attach = 0;
    fscanf(f, "attach to process=%d\n", &attach);
    m_bAttachToProcess = !!attach;
  } else {
    m_bAttachToProcess = false;
  }

  if (fileVer >= 8) {
    int stop = 1;
    fscanf(f, "abort stack walk if address outside known modules=%d\n", &stop);
    m_bStopAtPCOutsideModules = !!stop;
  } else {
    m_bStopAtPCOutsideModules = true;
  }
  
#undef ERR
  fclose(f);
  m_bChanged = false;
  return true;
}

wxString ProfilerSettings::DoAbbreviations(wxString input) {
  for (std::map<wxString, wxString>::iterator it = m_symbolAbbreviations.begin();
       it != m_symbolAbbreviations.end(); ++it) {
    input.Replace(it->first, it->second);
  }
  return input;       
}