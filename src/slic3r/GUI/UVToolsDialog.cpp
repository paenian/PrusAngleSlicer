///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "UVToolsDialog.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/filedlg.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "GUI_App.hpp"
#include "I18N.hpp"
#include "UVToolsIntegration.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Format/UVTools/UVToolsService.hpp"

namespace Slic3r { namespace GUI {

namespace {

wxString u8(const std::string &s) { return wxString::FromUTF8(s.c_str()); }

class UVToolsDialog : public wxDialog
{
public:
    explicit UVToolsDialog(wxWindow *parent)
        : wxDialog(parent, wxID_ANY, _L("UVtools required for this printer"),
                   wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        , m_alive(std::make_shared<std::atomic<bool>>(true))
    {
        auto *top = new wxBoxSizer(wxVERTICAL);

        m_body = new wxStaticText(this, wxID_ANY, wxString());
        top->Add(m_body, 0, wxALL | wxEXPAND, 14);

        m_gauge = new wxGauge(this, wxID_ANY, 1000, wxDefaultPosition, wxSize(480, 18));
        top->Add(m_gauge, 0, wxLEFT | wxRIGHT | wxEXPAND, 14);

        m_status = new wxStaticText(this, wxID_ANY, wxString());
        top->Add(m_status, 0, wxALL | wxEXPAND, 14);

        auto *buttons = new wxBoxSizer(wxHORIZONTAL);
        m_btn_download = new wxButton(this, wxID_ANY, _L("Download and install"));
        m_btn_browse   = new wxButton(this, wxID_ANY, _L("Use my own UVtools") + wxString::FromUTF8("\xE2\x80\xA6"));
        m_btn_skip     = new wxButton(this, wxID_ANY, _L("Export .sl1 instead"));
        m_btn_cancel   = new wxButton(this, wxID_ANY, _L("Cancel"));
        for (wxButton *b : { m_btn_download, m_btn_browse, m_btn_skip })
            buttons->Add(b, 0, wxRIGHT, 8);
        buttons->AddStretchSpacer(1);
        buttons->Add(m_btn_cancel, 0);
        top->Add(buttons, 0, wxALL | wxEXPAND, 14);

        SetSizer(top);

        m_btn_download->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { start_download(); });
        m_btn_browse->Bind(wxEVT_BUTTON,   [this](wxCommandEvent &) { browse_for_existing(); });
        m_btn_skip->Bind(wxEVT_BUTTON,     [this](wxCommandEvent &) { finish(UVToolsPrep::Skipped); });
        m_btn_cancel->Bind(wxEVT_BUTTON,   [this](wxCommandEvent &) { request_cancel(); });
        Bind(wxEVT_CLOSE_WINDOW,           [this](wxCloseEvent &ev) {
            if (m_running) { request_cancel(); ev.Veto(); }   // wait for the worker to stop
            else           { finish(UVToolsPrep::Cancelled); }
        });

        show_confirm(wxString());
    }

    ~UVToolsDialog() override
    {
        *m_alive = false;
        m_cancel = true;
        if (m_worker.joinable())
            m_worker.join();
    }

    UVToolsPrep result() const { return m_result; }

private:
    // ---- states -----------------------------------------------------------
    void show_confirm(const wxString &note)
    {
        wxString text =
            _L("Exporting for this printer's native file format needs UVtools, a separate program that "
               "converts PrusaSlicer's slice output. It is not installed yet.") + "\n\n" +
            wxString::Format(_L("PrusaSlicer can download UVtools v%s (about 75 MB) from its official "
                                "GitHub release, verify its checksum, and keep it here:"),
                             u8(uvtools_pinned_version())) + "\n" +
            u8(uvtools_install_dir()) + "\n\n" +
            _L("UVtools is free software under the AGPL-3.0 license (github.com/sn4k3/UVtools). "
               "PrusaSlicer runs it as a separate program and does not bundle it.");
        if (! note.IsEmpty())
            text = note + "\n\n" + text;
        set_body(text);
        m_gauge->Hide();
        m_status->SetLabel(wxString());
        m_btn_download->SetLabel(_L("Download and install"));
        show_buttons(true);
        relayout();
    }

    void show_working()
    {
        set_body(wxString::Format(_L("Installing UVtools v%s"), u8(uvtools_pinned_version())));
        m_gauge->SetValue(0);
        m_gauge->Show();
        m_status->SetLabel(_L("Starting") + wxString::FromUTF8("\xE2\x80\xA6"));
        show_buttons(false);
        m_btn_cancel->Show();
        m_btn_cancel->Enable();
        relayout();
    }

    void show_failed(const std::string &reason)
    {
        set_body(_L("UVtools could not be installed.") + "\n\n" + u8(reason) + "\n\n" +
                 _L("You can retry, point PrusaSlicer at an existing UVtools, or export a plain .sl1 file "
                    "that you can convert yourself."));
        m_gauge->Hide();
        m_status->SetLabel(wxString());
        m_btn_download->SetLabel(_L("Retry"));
        show_buttons(true);
        relayout();
    }

    // ---- actions ----------------------------------------------------------
    void start_download()
    {
        if (m_running) return;
        m_cancel  = false;
        m_running = true;
        show_working();

        // Build the service on the GUI thread (reads AppConfig), run it on a worker.
        auto service = std::make_shared<uvtools::UVToolsService>(make_uvtools_service());
        auto alive   = m_alive;
        if (m_worker.joinable())
            m_worker.join();
        m_worker = std::thread([this, service, alive]() {
            auto progress = [this, alive](double frac, const std::string &msg) {
                wxGetApp().CallAfter([this, alive, frac, msg]() {
                    if (*alive) on_progress(frac, msg);
                });
                return ! m_cancel.load();
            };
            uvtools::ConvertResult r = service->ensure_available(progress);
            wxGetApp().CallAfter([this, alive, r]() {
                if (*alive) on_worker_done(r);
            });
        });
    }

    void on_progress(double frac, const std::string &msg)
    {
        m_gauge->SetValue(int(std::max(0.0, std::min(1.0, frac)) * 1000.0));
        m_status->SetLabel(u8(msg));
    }

    void on_worker_done(const uvtools::ConvertResult &r)
    {
        m_running = false;
        if (r.ok()) {
            refresh_uvtools_paths();        // link: writer now sees the remembered path
            BOOST_LOG_TRIVIAL(info) << "[UVtools] Installed and linked: " << r.output_path;
            finish(UVToolsPrep::Ready);
        } else if (m_cancel) {
            show_confirm(_L("Download cancelled."));
        } else {
            show_failed(r.message);
        }
    }

    void browse_for_existing()
    {
        wxFileDialog dlg(this, _L("Select the UVtools command-line program (UVtoolsCmd)"),
                         wxString(), wxString(), wxFileSelectorDefaultWildcardStr,
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK)
            return;
        const std::string path = dlg.GetPath().ToUTF8().data();
        if (AppConfig *cfg = wxGetApp().app_config) {
            cfg->set("", "uvtools_executable_path", path);
            cfg->save();
        }
        refresh_uvtools_paths();
        BOOST_LOG_TRIVIAL(info) << "[UVtools] Linked user-selected executable: " << path;
        finish(UVToolsPrep::Ready);
    }

    void request_cancel()
    {
        if (m_running) {
            m_cancel = true;                 // worker stops at its next progress tick
            m_btn_cancel->Disable();
            m_status->SetLabel(_L("Cancelling") + wxString::FromUTF8("\xE2\x80\xA6"));
        } else {
            finish(UVToolsPrep::Cancelled);
        }
    }

    void finish(UVToolsPrep result)
    {
        m_result = result;
        EndModal(wxID_OK);
    }

    // ---- layout helpers ---------------------------------------------------
    void set_body(const wxString &text)
    {
        m_body->SetLabel(text);
        m_body->Wrap(480);
    }
    void show_buttons(bool all)
    {
        for (wxButton *b : { m_btn_download, m_btn_browse, m_btn_skip, m_btn_cancel }) {
            b->Show(all);
            b->Enable();
        }
    }
    void relayout()
    {
        Layout();
        GetSizer()->SetSizeHints(this);
        Fit();
        CentreOnParent();
    }

    wxStaticText *m_body = nullptr, *m_status = nullptr;
    wxGauge      *m_gauge = nullptr;
    wxButton     *m_btn_download = nullptr, *m_btn_browse = nullptr,
                 *m_btn_skip = nullptr,     *m_btn_cancel = nullptr;

    std::thread                          m_worker;
    std::atomic<bool>                    m_cancel{false};
    std::atomic<bool>                    m_running{false};
    std::shared_ptr<std::atomic<bool>>   m_alive;   // guards CallAfter lambdas after destruction
    UVToolsPrep                          m_result = UVToolsPrep::Cancelled;
};

} // namespace

UVToolsPrep ensure_uvtools_ready(wxWindow *parent)
{
    refresh_uvtools_paths();            // pick up Preferences changes
    if (uvtools_is_available())
        return UVToolsPrep::Ready;

    UVToolsDialog dlg(parent);
    dlg.ShowModal();
    return dlg.result();
}

}} // namespace Slic3r::GUI
