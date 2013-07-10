require "test_helper"

class TestCommit < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_read_commit_data
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    assert_equal obj.oid, oid
    assert_equal obj.type, :commit
    assert_equal obj.message, "testing\n"
    assert_equal obj.time.to_i, 1273360386
    assert_equal obj.epoch_time, 1273360386

    c = obj.committer
    assert_equal c[:name], "Scott Chacon"
    assert_equal c[:time].to_i, 1273360386
    assert_equal c[:email], "schacon@gmail.com"

    c = obj.author
    assert_equal c[:name], "Scott Chacon"
    assert_equal c[:time].to_i, 1273360386
    assert_equal c[:email], "schacon@gmail.com"

    assert_equal obj.tree.oid, "181037049a54a1eb5fab404658a3a250b44335d7"
    assert_equal [], obj.parents
  end

  def test_commit_with_multiple_parents
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    obj = @repo.lookup(oid)
    parents = obj.parents.map {|c| c.oid }
    assert parents.include?("9fd738e8f7967c078dceed8190330fc8648ee56a")
    assert parents.include?("c47800c7266a2be04c571c04d5a6614691ea99bd")
  end

  def test_get_parent_oids
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    obj = @repo.lookup(oid)
    parents = obj.parent_oids
    assert parents.include?("9fd738e8f7967c078dceed8190330fc8648ee56a")
    assert parents.include?("c47800c7266a2be04c571c04d5a6614691ea99bd")
  end

  def test_get_tree_oid
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    assert_equal obj.tree_oid, "181037049a54a1eb5fab404658a3a250b44335d7"
  end
end

class CommitWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_write_a_commit
    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    Rugged::Commit.create(@repo,
      :message => "This is the commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person,
      :parents => [@repo.head.target],
      :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
  end

  def test_write_commit_with_time_offset
    person = {:name => 'Jake', :email => 'jake@github.com', :time => Time.now, :time_offset => 3600}

    oid = Rugged::Commit.create(@repo,
      :message => "This is the commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person,
      :parents => [@repo.head.target],
      :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")

    commit = @repo.lookup(oid)
    assert_equal 3600, commit.committer[:time].utc_offset
  end

  def test_write_invalid_parents
    person = {:name => 'Jake', :email => 'jake@github.com', :time => Time.now, :time_offset => 3600}

    assert_raises TypeError do
      Rugged::Commit.create(@repo,
        :message => "This is the commit message\n\nThis commit is created from Rugged",
        :committer => person,
        :author => person,
        :parents => [:invalid_parent],
        :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
    end
  end

  def test_write_empty_email
    person = {:name => 'Jake', :email => '', :time => Time.now}
    Rugged::Commit.create(@repo,
      :message => "This is the commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person,
      :parents => [@repo.head.target],
      :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
  end
end
