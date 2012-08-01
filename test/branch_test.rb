require "test_helper"

context "Rugged::Repository branch stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git'
    @repo = Rugged::Repository.new(@path)
  end

  teardown do
    FileUtils.remove_entry_secure(@path + '/refs/heads/foo_branch', true)
    FileUtils.remove_entry_secure(@path + '/refs/heads/foo', true)
  end

  test "can create a branch" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo_branch", ref.target )
    branches = @repo.refs(/foo_branch/)
    assert_equal branches.size, 1
    assert_equal branches[0].target, ref.target
  end

  test "should check the branch name when creating" do
    assert_raise(TypeError){@repo.create_branch(1, "c0004" )}
  end

  test "should check the target when creating" do
    assert_raise(TypeError){@repo.create_branch("foo_branch", 1)}
  end

  test "should check the target is valid when creating" do
    assert_raise(Rugged::InvalidError){
      @repo.create_branch("foo_branch", "c004")
    }
  end

  test "should check the target is existing when creating" do
    assert_raise(Rugged::OdbError){
      @repo.create_branch("foo_branch", "8496071c1b46c854b31185ea97743be6a877447a")
    }
  end

  # 0c37a5391bbff43c37f0d0371823a5509eed5b1d tag
  # c47800c7266a2be04c571c04d5a6614691ea99bd commit
  # a8233120f6ad708f843d861ce2b7228ec4e3dec6 blob
  test "should target is a tag or commit before creating a branch" do
    assert_raise(TypeError){
      @repo.create_branch("foo_branch", "a8233120f6ad708f843d861ce2b7228ec4e3dec6")
    }
  end

  test "forbids to re-create an already existing branch" do
    @repo.create_branch("foo_branch", "0c37a5391bbff43c37f0d0371823a5509eed5b1d")
    assert_raise(Rugged::ReferenceError) {
      @repo.create_branch("foo_branch", "0c37a5391bbff43c37f0d0371823a5509eed5b1d")
    }
  end

  test "allows to force re-creation an already existing branch" do
    @repo.create_branch("foo_branch", "0c37a5391bbff43c37f0d0371823a5509eed5b1d")
    oid = @repo.create_branch("foo_branch", "0c37a5391bbff43c37f0d0371823a5509eed5b1d",true)
    assert_equal oid, "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
  end

  test "can rename a branch" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.move_branch("foo", "foo_branch")
    branches = @repo.refs(/foo_branch/)
    assert_equal branches.size, 1
    assert_equal branches[0].target, ref.target
    assert_empty @repo.refs(/foo$/)
  end

  test "should detect an invalid 'old name' branch when renaming/moving" do
    assert_raise(TypeError){@repo.move_branch(1,"foo_branch")}
  end

  test "should detect an invalid 'new name' branch when renaming/moving" do
    assert_raise(TypeError){@repo.move_branch("foo",1)}
  end

  test "should detect an inexisting 'old name' branch when renaming/moving" do
    assert_raise(Rugged::ReferenceError){@repo.move_branch("fii", "foo_branch")}
  end

  test "forbids overwriting existing branch is not forced to" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.create_branch("foo_branch", ref.target )
    assert_raise(Rugged::ReferenceError){@repo.move_branch("foo", "foo_branch")}
  end

  test "forbids overwriting existing branch when forced to" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.create_branch("foo_branch", ref.target )
    @repo.move_branch("foo", "foo_branch", true)
    branches = @repo.refs(/foo_branch/)
    assert_equal branches.size, 1
    assert_equal branches[0].target, ref.target
    assert_empty @repo.refs(/foo$/)
  end

  test "can delete a branch (default type)" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.delete_branch("foo")
    branches = @repo.refs(/foo$/)
    assert_empty branches
  end

  test "should check the name of the branch is of right type before deleting" do
    assert_raise(TypeError){@repo.delete_branch(1)}
  end

  test "should check the type of the branch before deleting" do
    assert_raise(TypeError){@repo.delete_branch("foo", :foo)}
  end

  test "can delete a local branch" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.delete_branch("foo", :local)
    branches = @repo.refs(/foo$/)
    assert_empty branches
  end

  test "can iterate on branch names" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    @repo.create_branch("foo", ref.target )
    @repo.create_branch("foo_branch", ref.target )
    branch_names = []
    @repo.each_branch do |name|
      branch_names << name
    end
    assert_equal branch_names.size, 4
#    assert branch_names.include?("master"), "Branch list does not include 'master'"
#    assert branch_names.include?("packed"), "Branch list does not include 'packed'"
#    assert branch_names.include?("foo"), "Branch list does not include 'foo'"
#    assert branch_names.include?("foo_branch"), "Branch list does not include 'foo_branch'"
  end

end
