///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP
#define SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP

#include <string>

#include "libslic3r/Format/UVTools/UVToolsService.hpp"

namespace Slic3r { namespace GUI {

// Wire the libslic3r UVtools convert writer to the GUI boundary:
//  - supply UVtools paths from AppConfig (global uvtools_executable_path / cached)
//  - supply an Http-backed download function (keeps libslic3r GUI-agnostic)
//  - persist the resolved/cached path back to AppConfig
//  - route non-fatal conversion warnings to the user
//  - resolve the pinned UVtools release for the current platform/arch
// Call once during GUI app init.
void init_uvtools_integration();

// Re-read the UVtools paths from AppConfig into the export writer. Call after
// anything changes them (Preferences, the download dialog) so the writer does not
// keep a stale copy.
void refresh_uvtools_paths();

// True if a UVtools executable is already usable (override or remembered path
// exists on disk). Does not touch the network.
bool uvtools_is_available();

// A service configured with the current AppConfig paths, the pinned release and
// the Http download. Used by the download dialog (runs on a worker thread).
uvtools::UVToolsService make_uvtools_service();

// Where UVtools gets installed (<data_dir>/uvtools/v<version>) and which version.
std::string uvtools_install_dir();
std::string uvtools_pinned_version();

}} // namespace Slic3r::GUI

#endif // SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP
