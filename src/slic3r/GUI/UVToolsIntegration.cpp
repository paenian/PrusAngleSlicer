///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "UVToolsIntegration.hpp"

#include <fstream>
#include <string>

#include <boost/log/trivial.hpp>

#include "GUI_App.hpp"
#include "Plater.hpp"
#include "NotificationManager.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Format/UVTools/SLAConvertArchiveWriter.hpp"
#include "libslic3r/Format/UVTools/UVToolsService.hpp"
#include "slic3r/Utils/Http.hpp"

namespace Slic3r { namespace GUI {

// --- Pinned UVtools release (U1 data). Update version + all SHA-256 together. ---
// Pinned against UVtools v7.1.2 release assets (verified by downloading each
// artifact and computing its SHA-256). The downloaded archive is verified
// against these before it is ever extracted or executed (SECURITY-13).
namespace {
constexpr const char *UVTOOLS_PINNED_VERSION = "7.1.2";

// SHA-256 of each official UVtools_<os>-<arch>_v7.1.2.zip release asset.
// (Only the current platform's constant is referenced per build; the rest are
//  intentionally retained for the other build targets.)
[[maybe_unused]] constexpr const char *SHA_OSX_ARM64   = "0fe329344102ab0326c143ea9200ec6f0fdb9708e747c178bc34b6e46cc0961f";
[[maybe_unused]] constexpr const char *SHA_OSX_X64     = "ffb14e9055d85b80c2c8556b6573d5560d911257f2eddc61d688dc7b9bedebc6";
[[maybe_unused]] constexpr const char *SHA_WIN_X64     = "8ed807737242738c8772c966f8bd302a214764e439e1e24893fbf9b65a542f41";
[[maybe_unused]] constexpr const char *SHA_WIN_ARM64   = "dcbbb9c0032c71bb39e60683dea5acee6ba10a4c5ceff18c00f2d005351e14a5";
[[maybe_unused]] constexpr const char *SHA_LINUX_X64   = "4d06ec7e86043c409d81126311c5c8e6f4629eb5bb9d3f092d8ec2401d157c1c";
[[maybe_unused]] constexpr const char *SHA_LINUX_ARM64 = "6cdd0b70bd4495c3cd974096bacdac5c57f2801a781f6cd43237a6355edf1b5a";

std::string release_url(const char *os_arch)
{
    return std::string("https://github.com/sn4k3/UVtools/releases/download/v")
         + UVTOOLS_PINNED_VERSION + "/UVtools_" + os_arch + "_v"
         + UVTOOLS_PINNED_VERSION + ".zip";
}

uvtools::UVToolsRelease pinned_release_for_current_arch()
{
    uvtools::UVToolsRelease rel;
    rel.version = UVTOOLS_PINNED_VERSION;

#if defined(_WIN32)
    // Windows: zip root contains UVtoolsCmd.exe.
    rel.entry_relpath = "UVtoolsCmd.exe";
    #if defined(_M_ARM64) || defined(__aarch64__)
        rel.url = release_url("win-arm64");  rel.sha256 = SHA_WIN_ARM64;
    #else
        rel.url = release_url("win-x64");    rel.sha256 = SHA_WIN_X64;
    #endif
#elif defined(__APPLE__)
    // macOS: zip contains UVtools.app bundle.
    rel.entry_relpath = "UVtools.app/Contents/MacOS/UVtoolsCmd";
    #if defined(__aarch64__)
        rel.url = release_url("osx-arm64");  rel.sha256 = SHA_OSX_ARM64;
    #else
        rel.url = release_url("osx-x64");    rel.sha256 = SHA_OSX_X64;
    #endif
#else
    // Linux: zip root contains UVtoolsCmd.
    rel.entry_relpath = "UVtoolsCmd";
    #if defined(__aarch64__)
        rel.url = release_url("linux-arm64"); rel.sha256 = SHA_LINUX_ARM64;
    #else
        rel.url = release_url("linux-x64");   rel.sha256 = SHA_LINUX_X64;
    #endif
#endif
    return rel;
}

uvtools::UVToolsPaths paths_from_appconfig()
{
    uvtools::UVToolsPaths p;
    AppConfig *cfg = wxGetApp().app_config;
    if (cfg) {
        if (cfg->has("", "uvtools_executable_path"))
            p.override_path = cfg->get("", "uvtools_executable_path");
        if (cfg->has("", "uvtools_cached_path"))
            p.cached_path = cfg->get("", "uvtools_cached_path");
    }
    return p;
}

// Http-backed download: fetch url -> dest file. Reports coarse progress.
bool download_file(const std::string &url, const std::string &dest,
                   const uvtools::ProgressFn &progress, std::string &error_out)
{
    bool ok = false;
    std::string body;
    Http::get(url)
        .size_limit(512 * 1024 * 1024) // UVtools release zips are tens of MB; allow up to 512 MB
        .on_progress([&progress](Http::Progress p, bool &cancel) {
            if (progress && p.dltotal > 0) {
                double frac = double(p.dlnow) / double(p.dltotal);
                if (! progress(frac * 0.85, "Downloading UVtools..."))
                    cancel = true;
            }
        })
        .on_complete([&](std::string data, unsigned /*status*/) {
            std::ofstream out(dest, std::ios::binary | std::ios::trunc);
            out.write(data.data(), std::streamsize(data.size()));
            ok = out.good();
        })
        .on_error([&](std::string /*body*/, std::string err, unsigned status) {
            error_out = err.empty() ? ("HTTP status " + std::to_string(status)) : err;
            ok = false;
        })
        .perform_sync();
    return ok;
}

void persist_cached_path(const std::string &resolved)
{
    if (AppConfig *cfg = wxGetApp().app_config) {
        cfg->set("", "uvtools_cached_path", resolved);
        cfg->save();
    }
}

void show_warning(const std::string &msg)
{
    // Non-fatal: log AND surface a visible GUI notification so the user sees the
    // reason (e.g. download/convert failure). Runs on the background slicing
    // thread, so marshal onto the GUI thread.
    BOOST_LOG_TRIVIAL(warning) << "[UVtools] " << msg;
    std::string text = "UVtools SLA conversion: " + msg;
    wxGetApp().CallAfter([text]() {
        if (auto *plater = wxGetApp().plater()) {
            if (auto *nm = plater->get_notification_manager())
                nm->push_notification(NotificationType::CustomNotification,
                                      NotificationManager::NotificationLevel::WarningNotificationLevel,
                                      text);
        }
    });
}

} // namespace

void init_uvtools_integration()
{
    SLAConvertArchiveWriter::set_paths(paths_from_appconfig());
    SLAConvertArchiveWriter::set_release(pinned_release_for_current_arch());
    SLAConvertArchiveWriter::set_download_fn(&download_file);
    SLAConvertArchiveWriter::set_persist_fn(&persist_cached_path);
    SLAConvertArchiveWriter::set_warning_fn(&show_warning);
}

}} // namespace Slic3r::GUI
