///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "SLAFormatMap.hpp"

#include <array>
#include <cctype>
#include <algorithm>

namespace Slic3r { namespace uvtools {

namespace {

// Known encoder-name -> default output extension + description.
// The encoder name is the strict UVtools FileFormat class name.
// Extensions follow UVtools' primary extension for that encoder.
struct KnownEncoder {
    const char *encoder_name;
    const char *extension;
    const char *description;
};

// A compact, extensible table. Adding a format here (data) is all that is
// needed to support a new UVtools target; no algorithm changes required.
constexpr std::array<KnownEncoder, 4> KNOWN_ENCODERS = {{
    { "ChituboxFile",  "ctb", "Chitubox CTB (Elegoo/Anycubic/others)" },
    { "GooFile",       "goo", "Elegoo GOO" },
    { "AnycubicFile",  "pwmx", "Anycubic Photon Workshop" },
    { "PhotonFile",    "photon", "Anycubic Photon (legacy)" },
}};

const KnownEncoder *find_known(const std::string &encoder_name)
{
    for (const auto &e : KNOWN_ENCODERS)
        if (encoder_name == e.encoder_name)
            return &e;
    return nullptr;
}

bool starts_with_prefix(const std::string &s)
{
    const std::string pfx = UVTOOLS_PREFIX;
    if (s.size() < pfx.size())
        return false;
    // Case-insensitive compare of the prefix only.
    for (std::size_t i = 0; i < pfx.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(pfx[i])))
            return false;
    }
    return true;
}

} // namespace

bool is_convert_target(const std::string &archive_format)
{
    return starts_with_prefix(archive_format);
}

std::optional<UVToolsTarget> lookup(const std::string &archive_format)
{
    if (! is_convert_target(archive_format))
        return std::nullopt;

    const std::string pfx = UVTOOLS_PREFIX;
    std::string body = archive_format.substr(pfx.size());

    // Split optional "@<version>".
    std::string encoder_name = body;
    unsigned int version = 0;
    auto at = body.find('@');
    if (at != std::string::npos) {
        encoder_name = body.substr(0, at);
        std::string vstr = body.substr(at + 1);
        if (vstr.empty())
            return std::nullopt; // "@" with no version -> malformed
        for (char c : vstr)
            if (! std::isdigit(static_cast<unsigned char>(c)))
                return std::nullopt; // non-numeric version -> malformed
        version = static_cast<unsigned int>(std::stoul(vstr));
        if (version == 0)
            return std::nullopt; // zero version -> malformed
    }

    if (encoder_name.empty())
        return std::nullopt; // empty encoder -> malformed

    UVToolsTarget t;
    t.encoder_name = encoder_name;
    t.version = version;
    if (const KnownEncoder *k = find_known(encoder_name)) {
        t.extension   = k->extension;
        t.description = k->description;
    } else {
        // Unknown-but-well-formed encoder: still allow it (UVtools may support it);
        // fall back to a lower-cased encoder name sans "File" as the extension guess.
        std::string ext = encoder_name;
        const std::string suffix = "File";
        if (ext.size() > suffix.size() &&
            ext.compare(ext.size() - suffix.size(), suffix.size(), suffix) == 0)
            ext = ext.substr(0, ext.size() - suffix.size());
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        t.extension   = ext;
        t.description = encoder_name;
    }
    return t;
}

std::vector<std::pair<std::string, UVToolsTarget>> supported_targets()
{
    std::vector<std::pair<std::string, UVToolsTarget>> out;
    out.reserve(KNOWN_ENCODERS.size());
    for (const auto &e : KNOWN_ENCODERS) {
        std::string id = std::string(UVTOOLS_PREFIX) + e.encoder_name;
        UVToolsTarget t;
        t.encoder_name = e.encoder_name;
        t.version      = 0;
        t.extension    = e.extension;
        t.description  = e.description;
        out.emplace_back(std::move(id), std::move(t));
    }
    return out;
}

}} // namespace Slic3r::uvtools
