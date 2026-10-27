///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "UVToolsService.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>

#include <miniz.h>

namespace process = boost::process;

namespace Slic3r { namespace uvtools {

// ---------------------------------------------------------------------------
// Minimal, self-contained SHA-256 (avoids adding OpenSSL to libslic3r).
// Standard FIPS 180-4 implementation.
// ---------------------------------------------------------------------------
namespace {

class Sha256 {
public:
    Sha256() { reset(); }

    void update(const uint8_t *data, size_t len)
    {
        for (size_t i = 0; i < len; ++i) {
            m_block[m_blocklen++] = data[i];
            if (m_blocklen == 64) {
                transform();
                m_bitlen += 512;
                m_blocklen = 0;
            }
        }
    }

    std::string hex_digest()
    {
        std::array<uint8_t, 32> hash{};
        finalize(hash.data());
        std::ostringstream os;
        os << std::hex << std::setfill('0');
        for (uint8_t b : hash)
            os << std::setw(2) << static_cast<int>(b);
        return os.str();
    }

private:
    uint8_t  m_block[64]{};
    uint32_t m_state[8]{};
    uint64_t m_bitlen = 0;
    uint32_t m_blocklen = 0;

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & (b | c)) | (b & c); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void reset()
    {
        m_bitlen = 0; m_blocklen = 0;
        m_state[0] = 0x6a09e667; m_state[1] = 0xbb67ae85;
        m_state[2] = 0x3c6ef372; m_state[3] = 0xa54ff53a;
        m_state[4] = 0x510e527f; m_state[5] = 0x9b05688c;
        m_state[6] = 0x1f83d9ab; m_state[7] = 0x5be0cd19;
    }

    void transform()
    {
        static const uint32_t K[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        uint32_t m[64];
        for (uint32_t i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (uint32_t(m_block[j]) << 24) | (uint32_t(m_block[j+1]) << 16) |
                   (uint32_t(m_block[j+2]) << 8) | uint32_t(m_block[j+3]);
        for (uint32_t i = 16; i < 64; ++i)
            m[i] = sig1(m[i-2]) + m[i-7] + sig0(m[i-15]) + m[i-16];

        uint32_t a=m_state[0],b=m_state[1],c=m_state[2],d=m_state[3];
        uint32_t e=m_state[4],f=m_state[5],g=m_state[6],h=m_state[7];
        for (uint32_t i = 0; i < 64; ++i) {
            uint32_t t1 = h + (rotr(e,6)^rotr(e,11)^rotr(e,25)) + choose(e,f,g) + K[i] + m[i];
            uint32_t t2 = (rotr(a,2)^rotr(a,13)^rotr(a,22)) + majority(a,b,c);
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        m_state[0]+=a; m_state[1]+=b; m_state[2]+=c; m_state[3]+=d;
        m_state[4]+=e; m_state[5]+=f; m_state[6]+=g; m_state[7]+=h;
    }

    void finalize(uint8_t *out)
    {
        uint64_t total_bits = m_bitlen + uint64_t(m_blocklen) * 8;
        uint32_t i = m_blocklen;
        m_block[i++] = 0x80;
        if (m_blocklen < 56) {
            while (i < 56) m_block[i++] = 0;
        } else {
            while (i < 64) m_block[i++] = 0;
            transform();
            std::memset(m_block, 0, 56);
        }
        for (int j = 7; j >= 0; --j)
            m_block[56 + (7 - j)] = uint8_t((total_bits >> (j * 8)) & 0xff);
        transform();
        for (uint32_t k = 0; k < 8; ++k) {
            out[k*4+0] = uint8_t((m_state[k] >> 24) & 0xff);
            out[k*4+1] = uint8_t((m_state[k] >> 16) & 0xff);
            out[k*4+2] = uint8_t((m_state[k] >> 8) & 0xff);
            out[k*4+3] = uint8_t(m_state[k] & 0xff);
        }
    }
};

} // namespace

std::optional<std::string> UVToolsService::sha256_file(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (! in)
        return std::nullopt;
    Sha256 h;
    std::vector<char> buf(64 * 1024);
    while (in) {
        in.read(buf.data(), buf.size());
        std::streamsize got = in.gcount();
        if (got > 0)
            h.update(reinterpret_cast<const uint8_t *>(buf.data()), static_cast<size_t>(got));
    }
    return h.hex_digest();
}

// ---------------------------------------------------------------------------
// Convert argument construction (pure; PBT target).
// ---------------------------------------------------------------------------
std::vector<std::string> build_convert_args(const std::string &src_sl1,
                                            const std::string &dst_path,
                                            const UVToolsTarget &target,
                                            bool no_overwrite)
{
    // Strict-encoder-name targeting avoids UVtools' shared-extension ambiguity.
    std::vector<std::string> args;
    args.reserve(8);
    args.emplace_back("convert");
    args.emplace_back(src_sl1);
    args.emplace_back(target.encoder_name);
    args.emplace_back(dst_path);
    if (target.version != 0) {
        args.emplace_back("-v");
        args.emplace_back(std::to_string(target.version));
    }
    if (no_overwrite)
        args.emplace_back("--no-overwrite");
    return args;
}

// ---------------------------------------------------------------------------
// Service
// ---------------------------------------------------------------------------
namespace {
bool file_exists(const std::string &p)
{
    try { return !p.empty() && boost::filesystem::exists(p); }
    catch (...) { return false; }
}

// Extract every entry of `zip_path` under `dest_dir`, preserving the archive's
// internal directory structure. Returns false on any error. Uses miniz.
bool extract_zip(const std::string &zip_path, const std::string &dest_dir, std::string &err)
{
    namespace fs = boost::filesystem;
    mz_zip_archive zip;
    std::memset(&zip, 0, sizeof(zip));
    if (! mz_zip_reader_init_file(&zip, zip_path.c_str(), 0)) {
        err = "Could not open UVtools archive for extraction.";
        return false;
    }
    bool ok = true;
    mz_uint n = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < n && ok; ++i) {
        mz_zip_archive_file_stat st;
        if (! mz_zip_reader_file_stat(&zip, i, &st)) { ok = false; err = "Corrupt entry in UVtools archive."; break; }

        // Normalize separators and guard against path traversal.
        std::string name = st.m_filename;
        std::replace(name.begin(), name.end(), '\\', '/');
        if (name.find("..") != std::string::npos) { continue; } // skip suspicious entries

        fs::path out = fs::path(dest_dir) / name;
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            boost::system::error_code ec; fs::create_directories(out, ec);
            continue;
        }
        boost::system::error_code ec; fs::create_directories(out.parent_path(), ec);
        if (! mz_zip_reader_extract_to_file(&zip, i, out.string().c_str(), 0)) {
            ok = false; err = "Failed to extract an entry from the UVtools archive."; break;
        }
    }
    mz_zip_reader_end(&zip);
    return ok;
}
} // namespace

std::optional<std::string> UVToolsService::resolve_executable() const
{
    if (file_exists(m_paths.override_path))
        return m_paths.override_path;
    if (file_exists(m_paths.cached_path))
        return m_paths.cached_path;
    return std::nullopt;
}

ConvertResult UVToolsService::ensure_available(const ProgressFn &progress)
{
    ConvertResult r;
    if (auto exe = resolve_executable()) {
        m_resolved = *exe;
        BOOST_LOG_TRIVIAL(info) << "[UVtools] Using existing executable: " << m_resolved;
        r.status = ConvertResult::Success;
        r.output_path = m_resolved;
        return r;
    }

    // Need to acquire. Requires a download function and a valid pinned release.
    if (! m_download || ! m_release.valid()) {
        r.status = ConvertResult::ToolMissing;
        r.message = "UVtools is not available and automatic download is not configured.";
        BOOST_LOG_TRIVIAL(warning) << "[UVtools] " << r.message
            << " (download_fn=" << (m_download ? "set" : "unset")
            << ", release_valid=" << (m_release.valid() ? "yes" : "no") << ")";
        return r;
    }

    BOOST_LOG_TRIVIAL(info) << "[UVtools] Acquiring UVtools v" << m_release.version
                            << " from " << m_release.url;

    namespace fs = boost::filesystem;
    fs::path tmp_zip;
    try {
        tmp_zip = fs::temp_directory_path() /
                  fs::unique_path("uvtools-%%%%-%%%%.zip");
    } catch (const std::exception &e) {
        r.status = ConvertResult::ToolMissing;
        r.message = std::string("Cannot create temp path for UVtools download: ") + e.what();
        return r;
    }

    std::string dl_err;
    if (progress) progress(0.0, "Downloading UVtools...");
    BOOST_LOG_TRIVIAL(info) << "[UVtools] Download started -> " << tmp_zip.string();
    if (! m_download(m_release.url, tmp_zip.string(), progress, dl_err)) {
        r.status = ConvertResult::ToolMissing;
        r.message = "Failed to download UVtools: " + dl_err;
        BOOST_LOG_TRIVIAL(error) << "[UVtools] Download failed: " << dl_err;
        return r;
    }
    BOOST_LOG_TRIVIAL(info) << "[UVtools] Download complete; verifying integrity...";

    // Verify integrity before doing anything with the artifact.
    if (progress) progress(0.9, "Verifying UVtools download...");
    auto got = sha256_file(tmp_zip.string());
    if (! got) {
        boost::system::error_code ec; fs::remove(tmp_zip, ec);
        r.status = ConvertResult::VerifyFailed;
        r.message = "Could not read the downloaded UVtools archive for verification.";
        return r;
    }
    std::string expected = m_release.sha256;
    // normalize to lowercase for comparison
    for (char &c : expected) c = char(std::tolower((unsigned char)c));
    if (*got != expected) {
        boost::system::error_code ec; fs::remove(tmp_zip, ec);
        r.status = ConvertResult::VerifyFailed;
        r.message = "UVtools download failed integrity verification (checksum mismatch). "
                    "The file was not executed.";
        BOOST_LOG_TRIVIAL(error) << "[UVtools] SHA-256 mismatch: expected " << expected
                                 << " got " << *got << " (download NOT executed)";
        return r;
    }
    BOOST_LOG_TRIVIAL(info) << "[UVtools] Integrity verified (SHA-256 ok); extracting...";

    // Extract the verified archive into a stable, versioned cache directory and
    // resolve the UVtoolsCmd entry inside it.
    if (progress) progress(0.95, "Extracting UVtools...");
    fs::path extract_dir;
    try {
        extract_dir = fs::temp_directory_path() /
                      ("uvtools-" + m_release.version);
        boost::system::error_code ec;
        fs::remove_all(extract_dir, ec);          // fresh extraction
        fs::create_directories(extract_dir, ec);
    } catch (const std::exception &e) {
        boost::system::error_code ec; fs::remove(tmp_zip, ec);
        r.status = ConvertResult::ToolMissing;
        r.message = std::string("Cannot prepare UVtools cache directory: ") + e.what();
        return r;
    }

    std::string ex_err;
    if (! extract_zip(tmp_zip.string(), extract_dir.string(), ex_err)) {
        boost::system::error_code ec; fs::remove(tmp_zip, ec);
        r.status = ConvertResult::ToolMissing;
        r.message = "Failed to extract UVtools: " + ex_err;
        return r;
    }
    { boost::system::error_code ec; fs::remove(tmp_zip, ec); } // drop the zip

    fs::path entry = extract_dir / m_release.entry_relpath;
    if (! fs::exists(entry)) {
        r.status = ConvertResult::ToolMissing;
        r.message = "UVtools CLI entry not found after extraction: " + m_release.entry_relpath;
        return r;
    }

#ifndef _WIN32
    // Ensure the CLI (and sibling helper) are executable on unix-likes.
    try {
        fs::permissions(entry,
            fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exe |
            fs::perms::others_read | fs::perms::others_exe);
    } catch (...) { /* best-effort */ }
#endif

    m_resolved = entry.string();
    if (m_persist) m_persist(m_resolved);
    if (progress) progress(1.0, "UVtools ready.");
    BOOST_LOG_TRIVIAL(info) << "[UVtools] Ready; executable resolved at: " << m_resolved;
    r.status = ConvertResult::Success;
    r.output_path = m_resolved;
    return r;
}

ConvertResult UVToolsService::convert(const std::string &src_sl1,
                                      const std::string &dst_path,
                                      const UVToolsTarget &target,
                                      unsigned int timeout_seconds)
{
    ConvertResult r;
    if (m_resolved.empty()) {
        if (auto exe = resolve_executable()) m_resolved = *exe;
    }
    if (m_resolved.empty()) {
        r.status = ConvertResult::ToolMissing;
        r.message = "UVtools executable not resolved.";
        return r;
    }

    std::vector<std::string> args = build_convert_args(src_sl1, dst_path, target);

    BOOST_LOG_TRIVIAL(info) << "[UVtools] Converting '" << src_sl1 << "' -> '" << dst_path
                            << "' as " << target.encoder_name
                            << (target.version ? ("@v" + std::to_string(target.version)) : "")
                            << " (timeout " << timeout_seconds << "s)";

    try {
        process::ipstream pstdout;
        process::ipstream pstderr;
        // Argument-vector invocation: no shell interpolation -> injection-safe.
        process::child child(m_resolved, process::args(args),
                             process::std_out > pstdout, process::std_err > pstderr);

        if (! child.wait_for(std::chrono::seconds(timeout_seconds))) {
            child.terminate();
            r.status = ConvertResult::Timeout;
            r.message = "UVtools conversion timed out after " + std::to_string(timeout_seconds) + "s.";
            BOOST_LOG_TRIVIAL(error) << "[UVtools] " << r.message;
            return r;
        }

        // Capture both streams for observability (T4). UVtools writes its
        // progress to stdout; errors/diagnostics may be on either stream.
        std::string line, outtext, errtext;
        while (std::getline(pstdout, line)) { outtext += line; outtext.push_back('\n'); }
        while (std::getline(pstderr, line)) { errtext += line; errtext.push_back('\n'); }

        int code = child.exit_code();
        if (! outtext.empty())
            BOOST_LOG_TRIVIAL(debug) << "[UVtools] stdout:\n" << outtext;
        if (! errtext.empty())
            BOOST_LOG_TRIVIAL(debug) << "[UVtools] stderr:\n" << errtext;

        if (code == 0 && boost::filesystem::exists(dst_path)) {
            r.status = ConvertResult::Success;
            r.output_path = dst_path;
            BOOST_LOG_TRIVIAL(info) << "[UVtools] Conversion succeeded: " << dst_path;
        } else {
            r.status = ConvertResult::ConvertFailed;
            // Prefer stderr, fall back to the tail of stdout, for the user message.
            std::string diag = ! errtext.empty() ? errtext : outtext;
            r.message = "UVtools conversion failed (exit code " + std::to_string(code) + ")." +
                        (diag.empty() ? "" : ("\n" + diag));
            BOOST_LOG_TRIVIAL(error) << "[UVtools] Conversion failed (exit code " << code << ")"
                                     << (diag.empty() ? "" : (": " + diag));
        }
    } catch (const std::exception &e) {
        r.status = ConvertResult::ConvertFailed;
        r.message = std::string("Failed to run UVtools: ") + e.what();
        BOOST_LOG_TRIVIAL(error) << "[UVtools] " << r.message;
    }
    return r;
}

ConvertResult UVToolsService::ensure_and_convert(const std::string &src_sl1,
                                                 const std::string &dst_path,
                                                 const UVToolsTarget &target,
                                                 const ProgressFn &progress,
                                                 unsigned int timeout_seconds)
{
    ConvertResult avail = ensure_available(progress);
    if (! avail.ok())
        return avail;
    return convert(src_sl1, dst_path, target, timeout_seconds);
}

}} // namespace Slic3r::uvtools
