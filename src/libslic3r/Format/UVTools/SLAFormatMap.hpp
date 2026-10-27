///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef SLA_FORMAT_MAP_HPP
#define SLA_FORMAT_MAP_HPP

#include <string>
#include <optional>
#include <vector>
#include <utility>

namespace Slic3r { namespace uvtools {

// Prefix that marks an sla_archive_format value as a UVtools-convert target.
// e.g. "uvtools:ChituboxFile@3", "uvtools:GooFile", "uvtools:AnycubicFile"
static constexpr const char *UVTOOLS_PREFIX = "uvtools:";

// A resolved UVtools conversion target parsed from an sla_archive_format value.
struct UVToolsTarget {
    // Strict UVtools encoder class name passed to `UVtoolsCmd convert <in> <encoder> <out>`.
    // Targeting the strict name (rather than a bare extension) avoids UVtools'
    // "extension shared by multiple encoders" error.
    std::string encoder_name;
    // Optional format version passed as `-v <version>`. 0 means "unspecified".
    unsigned int version = 0;
    // Output file extension (lower-case, no dot), used for the produced file.
    std::string extension;
    // Human-readable description (GUI/logging).
    std::string description;
};

// True iff the archive_format value is a UVtools-convert target (has the uvtools: prefix).
bool is_convert_target(const std::string &archive_format);

// Parse+validate a "uvtools:<EncoderName>[@<version>]" value.
// Returns nullopt if the value is not a uvtools: target OR is malformed
// (empty encoder name, non-numeric/zero version).
std::optional<UVToolsTarget> lookup(const std::string &archive_format);

// The set of known/bundled target descriptors, keyed by the full archive_format
// id (e.g. "uvtools:GooFile"). Used by the GUI to present choices and by tests.
std::vector<std::pair<std::string, UVToolsTarget>> supported_targets();

}} // namespace Slic3r::uvtools

#endif // SLA_FORMAT_MAP_HPP
