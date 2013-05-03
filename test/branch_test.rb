# encoding: UTF-8
require "test_helper"

class BranchTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_list_all_names
    assert_equal [
      "master",
      "origin/HEAD",
      "origin/master",
      "origin/packed",
    ], Rugged::Branch.each_name(@repo).sort
  end

  def test_list_only_local_branches
    assert_equal ["master"], Rugged::Branch.each_name(@repo, :local).sort
  end

  def test_list_only_remote_branches
    assert_equal [
      "origin/HEAD",
      "origin/master",
      "origin/packed",
    ], Rugged::Branch.each_name(@repo, :remote).sort
  end

  def test_get_latest_commit_in_branch
    tip = Rugged::Branch.lookup(@repo, "master").tip

    assert_kind_of Rugged::Commit, tip
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", tip.oid
  end

  def test_lookup_local_branch
    branch = Rugged::Branch.lookup(@repo, "master")
    refute_nil branch

    assert_equal "master", branch.name
    assert_equal "refs/heads/master", branch.canonical_name
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", branch.tip.oid
  end

  def test_lookup_remote_branches
    branch = Rugged::Branch.lookup(@repo, "origin/packed", :remote)
    refute_nil branch

    assert_equal "origin/packed", branch.name
    assert_equal "refs/remotes/origin/packed", branch.canonical_name
    assert_equal "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9", branch.tip.oid
  end

  def test_lookup_unicode_branch_name
    new_branch = @repo.create_branch("Ångström", "5b5b025afb0b4c913b4c338a42934a3863bf3644")
    refute_nil new_branch

    retrieved_branch = Rugged::Branch.lookup(@repo, "Ångström")
    refute_nil retrieved_branch

    assert_equal new_branch, retrieved_branch
  end

  def test_delete_branch
    branch = @repo.create_branch("test_branch")
    branch.delete!
    assert_nil Rugged::Branch.lookup(@repo, "test_branch")
  end

  def test_is_head
    assert Rugged::Branch.lookup(@repo, "master").head?
    refute Rugged::Branch.lookup(@repo, "origin/master", :remote).head?
    refute Rugged::Branch.lookup(@repo, "origin/packed", :remote).head?
    refute @repo.create_branch("test_branch").head?
  end

  def test_rename_branch
    branch = @repo.create_branch("test_branch")

    branch.move('other_branch')

    assert_nil Rugged::Branch.lookup(@repo, "test_branch")
    refute_nil Rugged::Branch.lookup(@repo, "other_branch")
  end

  def test_create_new_branch
    new_branch = @repo.create_branch("test_branch", "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid

    refute_nil @repo.branches.find { |p| p.name == "test_branch" }
  end

  def test_create_unicode_branch
    branch_name = "A\314\212ngstro\314\210m"
    new_branch = @repo.create_branch(branch_name, "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    refute_nil new_branch
    assert_equal branch_name, new_branch.name
    assert_equal "refs/heads/#{branch_name}", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid

    refute_nil @repo.branches.find { |p| p.name == branch_name }
  end

  def test_create_branch_short_sha
    new_branch = @repo.create_branch("test_branch", "5b5b025")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  def test_create_branch_from_tag
    new_branch = @repo.create_branch("test_branch", "refs/tags/v0.9")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  def test_create_branch_from_head
    new_branch = @repo.create_branch("test_branch")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", new_branch.tip.oid
  end

  def test_create_branch_explicit_head
    new_branch = @repo.create_branch("test_branch", "HEAD")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", new_branch.tip.oid
  end

  def test_create_branch_from_commit
    new_branch = @repo.create_branch("test_branch",
      Rugged::Commit.lookup(@repo, "5b5b025afb0b4c913b4c338a42934a3863bf3644"))

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  def test_create_branch_from_tree_fails
    assert_raises ArgumentError, Rugged::InvalidError do
      @repo.create_branch("test_branch",
        Rugged::Tree.lookup(@repo, "f60079018b664e4e79329a7ef9559c8d9e0378d1"))
    end
  end

  def test_create_branch_from_blob_fails
    assert_raises ArgumentError, Rugged::InvalidError do
      @repo.create_branch("test_branch",
        Rugged::Blob.lookup(@repo, "1385f264afb75a56a5bec74243be9b367ba4ca08"))
    end
  end

  def test_create_branch_from_unknown_ref_fails
    assert_raises Rugged::ReferenceError do
      @repo.create_branch("test_branch", "i_do_not_exist")
    end
  end

  def test_create_branch_from_unknown_commit_fails
    assert_raises Rugged::ReferenceError do
      @repo.create_branch("test_branch", "dd15de908706711b51b7acb24faab726d2b3cb16")
    end
  end

  def test_create_branch_from_non_canonical_fails
    assert_raises Rugged::ReferenceError do
      @repo.create_branch("test_branch", "packed")
    end
  end
end
