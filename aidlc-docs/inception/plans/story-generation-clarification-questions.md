# User Stories — Clarification Questions

Two of your answers lightly conflict and one needs a scope boundary. Please resolve so the story set is unambiguous.

## Clarification 1 — "Core only" (Q3=B) vs. including the UVtools override story (Q5=A)
You chose core-only scope (Q3=B) but also asked to include the power-user manual-override story (Q5=A). The override story is a supporting flow, so these partially conflict. Also, strictly "core only" would exclude the auto-download and failure-fallback stories — but the first export cannot work without UVtools being acquired at least once, and the fallback is what prevents a failed export from leaving the user with nothing.

Which scope do you actually want for the stories this pass?

A) Core + essential supporting flows: select/define printer, export→convert, UVtools auto-acquisition (needed for first run), failure fallback, AND the manual-override story (Q5). Settings/status-visibility kept minimal. (Reconciles Q3 and Q5; most complete.)

B) Core + auto-acquisition + fallback (these are essential to a working export), but DROP the manual-override story (treat Q5 as deferred). (Strict "core only".)

C) Just the bare happy path (select printer + export→convert), everything else deferred — including auto-acquisition and fallback as separate stories (they'd be folded into the export story's criteria instead).

D) Other (please describe after [Answer]: tag below)

[Answer]: A

## Clarification 2 — Experience-level personas (Q2=C): how many levels?
You chose experience-level personas. How many?

A) Two: a novice/casual resin user and an advanced/power user, plus the maintainer/developer (3 personas total)

B) Three: novice, intermediate, advanced, plus maintainer (4 total)

C) Other (please describe after [Answer]: tag below)

[Answer]: B
