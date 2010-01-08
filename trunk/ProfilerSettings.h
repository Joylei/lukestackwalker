#pragma once
#include <wx/string.h>
#include <list>
#include <map>

enum {CURRENT_VERSION = 8};

class ProfilerSettings {
public:
  ProfilerSettings() {
    m_sampleDepth = 0;
    m_samplingStartDelay = SAMPLINGTIME_MANUALCONTROL;
    m_samplingTime = SAMPLINGTIME_MANUALCONTROL;
    m_bChanged = false;
    m_bConnectToSymServer = true;
    m_bAttachToProcess = true;
    m_bStopAtPCOutsideModules = true;
  }
  bool m_bAttachToProcess;
  bool m_bChanged;
  bool m_bConnectToSymServer;
  wxString m_executable;
  wxString m_commandLineArgs;
  wxString m_currentDirectory;
  std::list<wxString> m_debugInfoPaths;
  int m_sampleDepth;
  int m_samplingStartDelay;
  int m_samplingTime;
  std::map<wxString, wxString> m_sourceFileSubstitutions;
  std::map<wxString, wxString> m_symbolAbbreviations;
  std::map<wxString, wxString> m_environmentVariables;
  bool m_bStopAtPCOutsideModules;

  enum { SAMPLINGTIME_MANUALCONTROL = -1};
  wxString m_settingsFileName;
  bool SaveAs(std::string fname);
  bool Load(std::string fname);
  wxString DoAbbreviations(wxString input);

  bool operator == (const ProfilerSettings &rhs) {
    if (m_executable != rhs.m_executable)
      return false;
    if (m_commandLineArgs != rhs.m_commandLineArgs)
      return false;
    if (m_currentDirectory != rhs.m_currentDirectory)
      return false;
    if (m_debugInfoPaths != rhs.m_debugInfoPaths)
      return false;
    if (m_sampleDepth != rhs.m_sampleDepth)
      return false;
    if (m_samplingStartDelay != rhs.m_samplingStartDelay)
      return false;
    if (m_samplingTime != rhs.m_samplingTime)
      return false;
    if (m_sourceFileSubstitutions != rhs.m_sourceFileSubstitutions)
      return false;
    if (m_symbolAbbreviations != rhs.m_symbolAbbreviations)
      return false;
    if (m_bConnectToSymServer != rhs.m_bConnectToSymServer)
      return false;
    if (m_environmentVariables != rhs.m_environmentVariables)
      return false;
    if (m_bStopAtPCOutsideModules != rhs.m_bStopAtPCOutsideModules)
      return false;
    return true;
  }
  bool operator != (const ProfilerSettings &rhs) {
    return !(*this == rhs);
  }

};

struct ProfilerProgressStatus {
  int secondsLeftToStart;
  bool bStartedSampling;
  bool bSamplingPaused;
  int secondsLeftToProfile;
  bool bFinishedSampling;
  ProfilerProgressStatus() {
    secondsLeftToStart = 0;
    secondsLeftToProfile = 0;
    bStartedSampling = false;
    bFinishedSampling = false;    
    bSamplingPaused = false;
  }
};