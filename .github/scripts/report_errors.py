#!/usr/bin/env python3
"""Turn the interesting lines of a failed build log into a GitHub error annotation.

    python3 .github/scripts/report_errors.py <log file> "<title>"

Job logs are only downloadable by repository admins, but annotations are shown on the
run page to everyone, so this makes build failures readable without logging in.
"""
import re
import sys

PATTERNS = re.compile(
    r"(error[: ]|error C\d|error LNK\d|error MSB\d|CMake Error|fatal error|FAILED|"
    r"No such file|not found|undefined (reference|symbol)|\*\*\* \[|configure: error|"
    r"Error \d+|returned .*exit status|ld: )",
    re.IGNORECASE,
)

def main() -> int:
    path, title = sys.argv[1], (sys.argv[2] if len(sys.argv) > 2 else "Build failed")
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            lines = [l.rstrip("\r\n") for l in f]
    except OSError as e:
        print(f"::error title={title}::could not read log {path}: {e}")
        return 0
    hits = [l for l in lines if PATTERNS.search(l)]
    # De-duplicate while keeping order; errors tend to repeat.
    seen, uniq = set(), []
    for l in hits:
        k = l.strip()
        if k and k not in seen:
            seen.add(k)
            uniq.append(l)
    body = ["--- matching lines (last 40) ---"] + uniq[-40:] + ["--- end of log (last 25 lines) ---"] + lines[-25:]
    text = "\n".join(l[:400] for l in body)
    # Workflow-command escaping: %, CR, LF.
    text = text.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::error title={title}::{text}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
