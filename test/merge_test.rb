require "test_helper"

class TrivialMergeTest < Rugged::SandboxedTestCase
  def test_2alt
    repo = sandbox_init("merge-resolve")

    ours = repo.rev_parse("trivial-2alt")
    theirs = repo.rev_parse("trivial-2alt-branch")
    base = repo.rev_parse(repo.merge_base(ours, theirs))

    index = ours.tree.merge(theirs.tree, base.tree)

    refute index.conflicts?
  end

  def test_4
    repo = sandbox_init("merge-resolve")

    ours = repo.rev_parse("trivial-4")
    theirs = repo.rev_parse("trivial-4-branch")
    base = repo.rev_parse(repo.merge_base(ours, theirs))

    index = ours.tree.merge(theirs.tree, base.tree)

    assert index.conflicts?
    assert 2, index.count { |entry| entry[:stage] > 0 }
    assert index["new-and-different.txt", 2]
    assert index["new-and-different.txt", 3]
  end
end
