///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef SLIC3R_GUI_UVTOOLS_DIALOG_HPP
#define SLIC3R_GUI_UVTOOLS_DIALOG_HPP

class wxWindow;

namespace Slic3r { namespace GUI {

enum class UVToolsPrep {
    Ready,      // UVtools is installed/linked; export can convert to the printer's format
    Skipped,    // user chose to export a plain .sl1 instead
    Cancelled,  // user aborted the export
};

// Make sure a usable UVtools is present before an export that needs it.
// If it already is, returns Ready immediately without showing anything.
// Otherwise shows a modal dialog that explains what UVtools is, downloads +
// verifies + installs it under the app data dir (with progress and Cancel) and
// links it (remembers the path), or lets the user point at their own install.
// Must be called on the GUI thread.
UVToolsPrep ensure_uvtools_ready(wxWindow *parent);

}} // namespace Slic3r::GUI

#endif // SLIC3R_GUI_UVTOOLS_DIALOG_HPP
