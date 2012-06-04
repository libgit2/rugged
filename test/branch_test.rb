# encoding: UTF-8
require File.expand_path "../test_helper", __FILE__

context "Rugged::Repository#create_branch" do
  setup do
    @path = test_repo("testrepo.git")
    @repo = Rugged::Repository.new(@path)
  end

  test "can create a new branch" do
    new_branch = @repo.create_branch("test_branch", "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid

    refute_nil @repo.branches.find { |p| p.name == "test_branch" }
  end

  test "can create branches with non 7-bit ASCII names" do
    new_branch = @repo.create_branch("Ångström", "5b5b025afb0b4c913b4c338a42934a3863bf3644")

    refute_nil new_branch
    assert_equal "Ångström", new_branch.name
    assert_equal "refs/heads/Ångström", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid

    refute_nil @repo.branches.find { |p| p.name == "Ångström" }
  end

  test "can create a new branch with an abbreviated sha" do
    new_branch = @repo.create_branch("test_branch", "5b5b025")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  test "can create a new branch from a tag name" do
    new_branch = @repo.create_branch("test_branch", "refs/tags/v0.9")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  test "can create a new branch from implicit head" do
    new_branch = @repo.create_branch("test_branch")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", new_branch.tip.oid
  end

  test "can create a new branch from explicit head" do
    new_branch = @repo.create_branch("test_branch", "HEAD")

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", new_branch.tip.oid
  end

  test "can create a new branch from a commit object" do
    new_branch = @repo.create_branch("test_branch", Rugged::Commit.lookup(@repo, "5b5b025afb0b4c913b4c338a42934a3863bf3644"))

    refute_nil new_branch
    assert_equal "test_branch", new_branch.name
    assert_equal "refs/heads/test_branch", new_branch.canonical_name

    refute_nil new_branch.tip
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", new_branch.tip.oid
  end

  test "can not create a new branch from a tree" do
    error = assert_raises RuntimeError do
      @repo.create_branch("test_branch", Rugged::Tree.lookup(@repo, "f60079018b664e4e79329a7ef9559c8d9e0378d1"))
    end
   end

  test "can not create a new branch from a blob" do
    error = assert_raises RuntimeError do
      @repo.create_branch("test_branch", Rugged::Blob.lookup(@repo, "1385f264afb75a56a5bec74243be9b367ba4ca08"))
    end
  end

  test "can not create a new branch from an unknown branch" do
    error = assert_raises RuntimeError do
      @repo.create_branch("test_branch", "i_do_not_exist")
    end
  end

  test "can not create a new branch from an unknown commit" do
    error = assert_raises RuntimeError do
      @repo.create_branch("test_branch", "dd15de908706711b51b7acb24faab726d2b3cb16")
    end
  end

  test "can not create a new branch from a non canonical branch name" do
    error = assert_raises RuntimeError do
      @repo.create_branch("test_branch", "packed")
    end
  end
end