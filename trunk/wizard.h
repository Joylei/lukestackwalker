


class ProfilerSettingsWizard : public wxWizard {
public:
    ProfilerSettingsWizard(wxWindow *frame, ProfilerSettings *settings, bool useSizer = true);
    wxWizardPage *GetFirstPage() const { return m_page1; }

private:
    wxWizardPageSimple *m_page1;
    ProfilerSettings *m_settings;
};
