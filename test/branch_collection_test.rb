# encoding: UTF-8
require File.expand_path "../test_helper", __FILE__

context "Rugged::Repository#branches" do
  setup do
    @path = test_repo("testrepo.git")
    @repo = Rugged::Repository.new(@path)
  end

  test "can list all branches in a bare repository" do
    list = @repo.branches.map { |b| b.canonical_name }.sort
    assert_equal [
      "refs/heads/master",
      "refs/remotes/origin/HEAD",
      "refs/remotes/origin/master",
      "refs/remotes/origin/packed"
    ], list
  end

  test "can look up local branches" do
    branch = @repo.branches["master"]
    refute_nil branch

    assert_equal "master", branch.name
    assert_equal "refs/heads/master", branch.canonical_name
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", branch.tip.oid
  end

  test "can look up a local branch by its canonical name" do
    branch = @repo.branches["refs/heads/master"]
    refute_nil branch

    assert_equal "master", branch.name
    assert_equal "refs/heads/packed", branch.canonical_name
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", branch.tip.oid
  end

  test "can look up remote branches" do
    branch = @repo.branches["origin/packed"]
    refute_nil branch

    assert_equal "origin/packed", branch.name
    assert_equal "refs/remotes/origin/packed", branch.canonical_name
    assert_equal "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9", branch.tip.oid
  end

  test "can look up a local branch by its canonical name" do
    branch = @repo.branches["refs/remotes/origin/packed"]
    refute_nil branch

    assert_equal "origin/packed", branch.name
    assert_equal "refs/remotes/origin/packed", branch.canonical_name
    assert_equal "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9", branch.tip.oid
  end

  test "can look up branches with non 7-bit ASCII characters" do
    new_branch = @repo.create_branch("Ångström", "5b5b025afb0b4c913b4c338a42934a3863bf3644")
    refute_nil new_branch

    retrieved_branch = @repo.branches["Ångström"]
    refute_nil retrieved_branch

    assert_equal new_branch, retrieved_branch
  end
end