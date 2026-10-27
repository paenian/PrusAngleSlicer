///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP
#define SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP

namespace Slic3r { namespace GUI {

// Wire the libslic3r UVtools convert writer to the GUI boundary:
//  - supply UVtools paths from AppConfig (global uvtools_executable_path / cached)
//  - supply an Http-backed download function (keeps libslic3r GUI-agnostic)
//  - persist the resolved/cached path back to AppConfig
//  - route non-fatal conversion warnings to the user
//  - resolve the pinned UVtools release for the current platform/arch
// Call once during GUI app init.
void init_uvtools_integration();

}} // namespace Slic3r::GUI

#endif // SLIC3R_GUI_UVTOOLS_INTEGRATION_HPP
