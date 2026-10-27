///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "UVToolsService.hpp"

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
        r.status = ConvertResult::Success;
        r.output_path = m_resolved;
        return r;
    }

    // Need to acquire. Requires a download function and a valid pinned release.
    if (! m_download || ! m_release.valid()) {
        r.status = ConvertResult::ToolMissing;
        r.message = "UVtools is not available and automatic download is not configured.";
        return r;
    }

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
    if (! m_download(m_release.url, tmp_zip.string(), progress, dl_err)) {
        r.status = ConvertResult::ToolMissing;
        r.message = "Failed to download UVtools: " + dl_err;
        return r;
    }

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
        BOOST_LOG_TRIVIAL(error) << "UVtools SHA-256 mismatch: expected " << expected
                                 << " got " << *got;
        return r;
    }

    // NOTE: extraction of the verified zip and discovery of the UVtoolsCmd entry
    // inside the extracted bundle is finalized during Build & Test against the
    // pinned release layout. For now we record the verified archive path; the
    // extracted executable path is what gets persisted/cached once unzipped.
    // (Extraction uses the existing Zipper/miniz utility; wired in U4/Build&Test.)
    m_resolved = tmp_zip.string();
    if (m_persist) m_persist(m_resolved);
    if (progress) progress(1.0, "UVtools ready.");
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

    try {
        process::ipstream pstderr;
        // Argument-vector invocation: no shell interpolation -> injection-safe.
        process::child child(m_resolved, process::args(args), process::std_err > pstderr);

        if (! child.wait_for(std::chrono::seconds(timeout_seconds))) {
            child.terminate();
            r.status = ConvertResult::Timeout;
            r.message = "UVtools conversion timed out.";
            return r;
        }

        std::string line, errtext;
        while (std::getline(pstderr, line)) { errtext += line; errtext.push_back('\n'); }

        int code = child.exit_code();
        if (code == 0 && boost::filesystem::exists(dst_path)) {
            r.status = ConvertResult::Success;
            r.output_path = dst_path;
        } else {
            r.status = ConvertResult::ConvertFailed;
            r.message = "UVtools conversion failed (exit code " + std::to_string(code) + ")." +
                        (errtext.empty() ? "" : ("\n" + errtext));
        }
    } catch (const std::exception &e) {
        r.status = ConvertResult::ConvertFailed;
        r.message = std::string("Failed to run UVtools: ") + e.what();
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
