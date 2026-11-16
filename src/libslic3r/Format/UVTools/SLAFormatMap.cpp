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

// Known encoders (strict UVtools FileFormat class names) with their DEFAULT output
// extension and a description. Adding a format here (data) is all that is needed
// to support a new UVtools target; no algorithm changes required.
struct KnownEncoder {
    const char *encoder_name;
    const char *extension;      // default extension when the id does not name one
    const char *description;
};

constexpr std::array<KnownEncoder, 5> KNOWN_ENCODERS = {{
    { "ChituboxFile",  "ctb",    "Chitubox CTB (Elegoo/Anycubic/others)" },
    { "GooFile",       "goo",    "Elegoo GOO" },
    { "AnycubicFile",  "pwmx",   "Anycubic Photon Workshop" },
    { "PhotonFile",    "photon", "Anycubic Photon (legacy)" },
    { "PHZFile",       "phz",    "Chitubox PHZ" },
}};

// One UVtools encoder can write several flavours, selected by the OUTPUT file
// extension (e.g. GooFile writes Elegoo ".goo" or Phrozen ".prz", and encodes the
// layers differently for ".prz"). These give a precise description per flavour.
struct KnownFlavour {
    const char *encoder_name;
    const char *extension;
    const char *description;
};

constexpr std::array<KnownFlavour, 4> KNOWN_FLAVOURS = {{
    { "GooFile",      "goo", "Elegoo GOO" },
    { "GooFile",      "prz", "Phrozen PRZ" },
    { "ChituboxFile", "ctb", "Chitubox CTB" },
    { "ChituboxFile", "cbddlp", "Chitubox CBDDLP" },
}};

const KnownEncoder *find_known(const std::string &encoder_name)
{
    for (const auto &e : KNOWN_ENCODERS)
        if (encoder_name == e.encoder_name)
            return &e;
    return nullptr;
}

const KnownFlavour *find_flavour(const std::string &encoder_name, const std::string &ext)
{
    for (const auto &f : KNOWN_FLAVOURS)
        if (encoder_name == f.encoder_name && ext == f.extension)
            return &f;
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

bool all_alnum(const std::string &s)
{
    return std::all_of(s.begin(), s.end(),
                       [](unsigned char c) { return std::isalnum(c) != 0; });
}

} // namespace

bool is_convert_target(const std::string &archive_format)
{
    return starts_with_prefix(archive_format);
}

// Grammar:  uvtools:<Encoder>[@<version>][.<ext>]
//   uvtools:GooFile            -> GooFile, default extension (goo)
//   uvtools:GooFile.prz        -> GooFile writing a Phrozen .prz
//   uvtools:ChituboxFile@4     -> ChituboxFile version 4, default extension (ctb)
//   uvtools:ChituboxFile@4.ctb -> same, extension spelled out
std::optional<UVToolsTarget> lookup(const std::string &archive_format)
{
    if (! is_convert_target(archive_format))
        return std::nullopt;

    const std::string pfx = UVTOOLS_PREFIX;
    std::string body = archive_format.substr(pfx.size());

    // Optional ".<ext>" suffix (extensions never contain '.', so the first '.' splits).
    std::string ext_override;
    auto dot = body.find('.');
    if (dot != std::string::npos) {
        ext_override = body.substr(dot + 1);
        body         = body.substr(0, dot);
        if (ext_override.empty() || ! all_alnum(ext_override))
            return std::nullopt; // "." with no/odd extension -> malformed
        std::transform(ext_override.begin(), ext_override.end(), ext_override.begin(),
                       [](unsigned char c) { return char(std::tolower(c)); });
    }

    // Optional "@<version>".
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

    const KnownEncoder *k = find_known(encoder_name);
    if (k) {
        t.extension   = ext_override.empty() ? std::string(k->extension) : ext_override;
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
                       [](unsigned char c){ return char(std::tolower(c)); });
        t.extension   = ext_override.empty() ? ext : ext_override;
        t.description = encoder_name;
    }
    // A precise per-extension description wins when we know the flavour.
    if (const KnownFlavour *f = find_flavour(t.encoder_name, t.extension))
        t.description = f->description;
    return t;
}

std::vector<std::pair<std::string, UVToolsTarget>> supported_targets()
{
    std::vector<std::pair<std::string, UVToolsTarget>> out;
    out.reserve(KNOWN_ENCODERS.size() + KNOWN_FLAVOURS.size());
    for (const auto &e : KNOWN_ENCODERS) {
        std::string id = std::string(UVTOOLS_PREFIX) + e.encoder_name;
        if (auto t = lookup(id))
            out.emplace_back(std::move(id), std::move(*t));
    }
    // Additional flavours that differ from the encoder's default extension.
    for (const auto &f : KNOWN_FLAVOURS) {
        const KnownEncoder *k = find_known(f.encoder_name);
        if (! k || std::string(k->extension) == f.extension)
            continue; // already listed as the encoder's default
        std::string id = std::string(UVTOOLS_PREFIX) + f.encoder_name + "." + f.extension;
        if (auto t = lookup(id))
            out.emplace_back(std::move(id), std::move(*t));
    }
    return out;
}

}} // namespace Slic3r::uvtools
