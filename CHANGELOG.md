*   `Rugged::Index#read_tree` now actually checks that the given object is a
    `Rugged::Tree` instance.

    Fixes #401.

    *Andy Delcambre*

*   Remove defunct `Rugged::Diff::Line#hunk` and `Rugged::Diff::Line#owner`.

    Fixes #390.

    *Arthur Schreiber*

*   Remove `Rugged::Diff#tree` and change `Rugged::Diff#owner` to return the
    repository that the `Rugged::Diff` object belongs to.

    We need to keep a reference from the `Rugged::Diff` to the repository to
    ensure that the underlying libgit2 data does not get freed accidentally.

    Fixes #389.

    *Arthur Schreiber*

*   Add `#additions` and `#deletions` to `Rugged::Patch`.

    *Mindaugas Mozūras*
