///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
// Tests for the UVtools SLA format mapping (U2) and convert-arg construction (U3).
// Example-based tests plus property-style loops over generated inputs.
// (Framework note: RapidCheck is the intended PBT framework per NFR-9; these
//  property loops are structured to port to rc::check() once RapidCheck is wired.)

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <random>

#include "libslic3r/Format/UVTools/SLAFormatMap.hpp"
#include "libslic3r/Format/UVTools/UVToolsService.hpp"

using namespace Slic3r::uvtools;

TEST_CASE("UVtools prefix detection", "[AngledSlicing][UVtools]")
{
    CHECK(is_convert_target("uvtools:GooFile"));
    CHECK(is_convert_target("uvtools:ChituboxFile@3"));
    CHECK(is_convert_target("UVTOOLS:GooFile")); // case-insensitive prefix
    CHECK_FALSE(is_convert_target("SL1"));
    CHECK_FALSE(is_convert_target("pwmo"));
    CHECK_FALSE(is_convert_target(""));
    CHECK_FALSE(is_convert_target("uvtool:GooFile")); // not the prefix
}

TEST_CASE("UVtools lookup parses encoder and version", "[AngledSlicing][UVtools]")
{
    auto a = lookup("uvtools:GooFile");
    REQUIRE(a.has_value());
    CHECK(a->encoder_name == "GooFile");
    CHECK(a->version == 0);
    CHECK(a->extension == "goo");

    auto b = lookup("uvtools:ChituboxFile@4");
    REQUIRE(b.has_value());
    CHECK(b->encoder_name == "ChituboxFile");
    CHECK(b->version == 4);
    CHECK(b->extension == "ctb");

    // Unknown but well-formed encoder: accepted, extension guessed.
    auto c = lookup("uvtools:MysteryFile");
    REQUIRE(c.has_value());
    CHECK(c->encoder_name == "MysteryFile");
    CHECK(c->extension == "mystery");
}

TEST_CASE("UVtools lookup rejects malformed values", "[AngledSlicing][UVtools]")
{
    CHECK_FALSE(lookup("SL1").has_value());          // not a uvtools target
    CHECK_FALSE(lookup("uvtools:").has_value());     // empty encoder
    CHECK_FALSE(lookup("uvtools:GooFile@").has_value());   // @ with no version
    CHECK_FALSE(lookup("uvtools:GooFile@0").has_value());  // zero version
    CHECK_FALSE(lookup("uvtools:GooFile@x").has_value());  // non-numeric version
}

TEST_CASE("UVtools property: is_convert_target <=> lookup(nonmalformed)", "[AngledSlicing][UVtools]")
{
    // Property: for the supported target set, is_convert_target is true AND
    // lookup yields a value that round-trips encoder+extension.
    for (const auto &kv : supported_targets()) {
        const std::string &id = kv.first;
        CHECK(is_convert_target(id));
        auto t = lookup(id);
        REQUIRE(t.has_value());
        CHECK(t->encoder_name == kv.second.encoder_name);
        CHECK(t->extension == kv.second.extension);
        CHECK_FALSE(t->extension.empty());
    }
}

TEST_CASE("UVtools property: generated well-formed ids always parse", "[AngledSlicing][UVtools]")
{
    // Property loop (RapidCheck-ready): random valid encoder names + versions
    // must always parse, and the parsed encoder/version must match the input.
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> len_dist(1, 12);
    std::uniform_int_distribution<int> ver_dist(0, 999);
    std::uniform_int_distribution<int> ch_dist(0, 51);
    auto rand_name = [&](int n) {
        std::string s;
        for (int i = 0; i < n; ++i) {
            int c = ch_dist(rng);
            s.push_back(char(c < 26 ? 'A' + c : 'a' + (c - 26)));
        }
        return s;
    };

    for (int iter = 0; iter < 500; ++iter) {
        std::string name = rand_name(len_dist(rng));
        int ver = ver_dist(rng);
        std::string id = "uvtools:" + name;
        if (ver > 0) id += "@" + std::to_string(ver);

        auto t = lookup(id);
        REQUIRE(t.has_value());
        CHECK(t->encoder_name == name);
        CHECK(t->version == (unsigned)(ver > 0 ? ver : 0));
        CHECK_FALSE(t->extension.empty());
    }
}

TEST_CASE("UVtools convert args are well-formed and injection-safe", "[AngledSlicing][UVtools]")
{
    UVToolsTarget t;
    t.encoder_name = "GooFile";
    t.version = 0;
    auto args = build_convert_args("/tmp/in.sl1", "/tmp/out.goo", t);
    // Expect: convert <in> <encoder> <out> --no-overwrite
    REQUIRE(args.size() == 5);
    CHECK(args[0] == "convert");
    CHECK(args[1] == "/tmp/in.sl1");
    CHECK(args[2] == "GooFile");
    CHECK(args[3] == "/tmp/out.goo");
    CHECK(args[4] == "--no-overwrite");

    UVToolsTarget tv;
    tv.encoder_name = "ChituboxFile";
    tv.version = 4;
    auto args2 = build_convert_args("/tmp/in.sl1", "/tmp/out.ctb", tv);
    // convert <in> <encoder> <out> -v 4 --no-overwrite
    REQUIRE(args2.size() == 7);
    CHECK(args2[4] == "-v");
    CHECK(args2[5] == "4");
}

TEST_CASE("UVtools convert args property: paths passed verbatim as separate argv entries",
          "[AngledSlicing][UVtools]")
{
    // Property: however nasty the path (spaces, quotes, semicolons), it appears
    // as a single, unmodified argv entry -> no shell injection possible.
    const std::vector<std::string> nasty = {
        "/tmp/a b.sl1",
        "/tmp/a';rm -rf ~'.sl1",
        "/tmp/a\"b.sl1",
        "/tmp/a$(whoami).sl1",
        "/tmp/a|b&c.sl1",
    };
    UVToolsTarget t; t.encoder_name = "GooFile"; t.version = 0;
    for (const auto &p : nasty) {
        auto args = build_convert_args(p, p + ".goo", t);
        CHECK(args[1] == p);            // input path verbatim
        CHECK(args[3] == p + ".goo");   // output path verbatim
        // encoder name is a fixed token, never the path
        CHECK(args[2] == "GooFile");
    }
}

TEST_CASE("UVtools SHA-256 matches known vectors", "[AngledSlicing][UVtools]")
{
    // Verify the vendored SHA-256 against a known file content.
    // (empty file -> known SHA-256 of empty input)
    // We write a temp file to exercise sha256_file.
    const char *path = "/tmp/uvtools_sha_test.bin";
    {
        FILE *f = std::fopen(path, "wb");
        REQUIRE(f != nullptr);
        const char *data = "abc";
        std::fwrite(data, 1, 3, f);
        std::fclose(f);
    }
    auto h = UVToolsService::sha256_file(path);
    REQUIRE(h.has_value());
    // SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    CHECK(*h == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    std::remove(path);
}
