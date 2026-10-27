///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
// Minimal property-based-testing helper on top of Catch2 v3.
//
// Why this exists: the repo uses Catch2 v3.8; RapidCheck's test integration is
// Catch2-v2-only and RapidCheck is not in the deps. Rather than vendor a sizable
// test-only library (against the minimal-footprint constraint), this provides the
// practical PBT guarantees we need:
//   - domain generators (caller-supplied),
//   - seed-based reproducibility via Catch::getSeed() (controlled by --rng-seed,
//     logged by Catch2 on every run),
//   - a minimal reproducing case: on failure the input is shrunk toward a
//     smaller counterexample before being reported.
//
// Not a full library-grade shrinker; sufficient for the string/path inputs here.

#ifndef UVTOOLS_PROPERTY_HPP
#define UVTOOLS_PROPERTY_HPP

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_get_random_seed.hpp>

#include <functional>
#include <random>
#include <string>
#include <sstream>

namespace pbt {

// Deterministic RNG seeded from Catch2's seed so a failing run is reproducible
// with `--rng-seed <value>` (Catch2 prints the seed each run).
inline std::mt19937 &rng()
{
    static thread_local std::mt19937 engine{ Catch::getSeed() };
    return engine;
}

// Run `prop` over `iterations` generated values from `gen`. `prop` returns true
// when the property holds. On the first failure, shrink via `shrink` (which
// yields a vector of "smaller" candidates) to find a minimal still-failing input,
// then FAIL with the minimal counterexample and the seed for reproduction.
//
// T must be streamable to std::ostream for the failure message.
template <class T, class Gen, class Prop, class Shrink>
void check(int iterations, Gen &&gen, Prop &&prop, Shrink &&shrink)
{
    for (int i = 0; i < iterations; ++i) {
        T value = gen();
        if (prop(value))
            continue;

        // Found a counterexample — shrink it.
        T minimal = value;
        bool shrunk = true;
        int guard = 0;
        while (shrunk && guard++ < 1000) {
            shrunk = false;
            for (T &cand : shrink(minimal)) {
                if (! prop(cand)) { minimal = cand; shrunk = true; break; }
            }
        }
        std::ostringstream os;
        os << "Property failed. Seed=" << Catch::getSeed()
           << " (reproduce with --rng-seed " << Catch::getSeed() << ").\n"
           << "Original counterexample: " << value << "\n"
           << "Minimal counterexample:  " << minimal;
        FAIL(os.str());
        return;
    }
    SUCCEED();
}

// Overload without shrinking (for properties over values that don't shrink
// meaningfully). Still reports the seed + counterexample.
template <class T, class Gen, class Prop>
void check(int iterations, Gen &&gen, Prop &&prop)
{
    check<T>(iterations, std::forward<Gen>(gen), std::forward<Prop>(prop),
             [](const T &) { return std::vector<T>{}; });
}

} // namespace pbt

#endif // UVTOOLS_PROPERTY_HPP
