///|/ Copyright (c) Prusa Research 2022 - 2023 Tomáš Mészáros @tamasmeszaros
///|/ Copyright (c) 2023 Mimoja @Mimoja
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "SLAArchiveWriter.hpp"

#include "SLAArchiveFormatRegistry.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "UVTools/SLAFormatMap.hpp"
#include "UVTools/SLAConvertArchiveWriter.hpp"

namespace Slic3r {

std::unique_ptr<SLAArchiveWriter>
SLAArchiveWriter::create(const std::string &archtype, const SLAPrinterConfig &cfg)
{
    // 3rd-party SLA printer support: "uvtools:<EncoderName>[@version]" ids route
    // to the convert writer (Strategy A), bypassing the fixed registry lookup
    // since the id is dynamic. Well-formed-ness is validated by uvtools::lookup;
    // a malformed value still yields the writer, which fails safe to an .sl1.
    if (uvtools::is_convert_target(archtype))
        return std::make_unique<SLAConvertArchiveWriter>(cfg);

    std::unique_ptr<SLAArchiveWriter> ret;
    auto factory = get_writer_factory(archtype.c_str());

    if (factory)
        ret = factory(cfg);

    return ret;
}

} // namespace Slic3r
