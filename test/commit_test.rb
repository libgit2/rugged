require "test_helper"

class TestCommit < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_lookup_raises_error_if_object_type_does_not_match
    assert_raises Rugged::InvalidError do
      # blob
      Rugged::Commit.lookup(@repo, "fa49b077972391ad58037050f2a75f74e3671e92")
    end

    assert_raises Rugged::InvalidError do
      # tag
      Rugged::Commit.lookup(@repo, "0c37a5391bbff43c37f0d0371823a5509eed5b1d")
    end

    assert_raises Rugged::InvalidError do
      # tree
      Rugged::Commit.lookup(@repo, "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
    end

    subclass = Class.new(Rugged::Commit)

    assert_raises Rugged::InvalidError do
      # blob
      subclass.lookup(@repo, "fa49b077972391ad58037050f2a75f74e3671e92")
    end

    assert_raises Rugged::InvalidError do
      # tag
      subclass.lookup(@repo, "0c37a5391bbff43c37f0d0371823a5509eed5b1d")
    end

    assert_raises Rugged::InvalidError do
      # tree
      subclass.lookup(@repo, "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")
    end
  end

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

  def test_amend_commit
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    entry = {:type => :blob,
             :name => "README.txt",
             :oid  => "1385f264afb75a56a5bec74243be9b367ba4ca08",
             :filemode => 33188}

    builder = Rugged::Tree::Builder.new
    builder << entry
    tree_oid = builder.write(@repo)

    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    commit_params = {
      :message => "This is the amended commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person,
      :tree => tree_oid
    }

    new_commit_oid = obj.amend(commit_params)

    amended_commit = @repo.lookup(new_commit_oid)
    assert_equal commit_params[:message], amended_commit.message
    assert_equal tree_oid, amended_commit.tree.oid
  end

  def test_amend_commit_blank_tree
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    commit_params = {
      :message => "This is the amended commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person
    }

    new_commit_oid = obj.amend(commit_params)

    amended_commit = @repo.lookup(new_commit_oid)
    assert_equal commit_params[:message], amended_commit.message
  end

  def test_amend_commit_blank_author_and_committer
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    commit_params = {
      :message => "This is the amended commit message\n\nThis commit is created from Rugged"
    }

    new_commit_oid = obj.amend(commit_params)

    amended_commit = @repo.lookup(new_commit_oid)
    assert_equal commit_params[:message], amended_commit.message
  end

  def test_amend_commit_blank_message
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    entry = {:type => :blob,
             :name => "README.txt",
             :oid  => "1385f264afb75a56a5bec74243be9b367ba4ca08",
             :filemode => 33188}

    builder = Rugged::Tree::Builder.new
    builder << entry
    tree_oid = builder.write(@repo)

    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    commit_params = {
      :committer => person,
      :author => person,
      :tree => tree_oid
    }

    new_commit_oid = obj.amend(commit_params)

    amended_commit = @repo.lookup(new_commit_oid)
    assert_equal tree_oid, amended_commit.tree.oid
  end
end

class CommitWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_write_commit_with_time
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

  def test_write_commit_without_time
    person = {:name => 'Jake', :email => 'jake@github.com'}

    oid = Rugged::Commit.create(@repo,
      :message => "This is the commit message\n\nThis commit is created from Rugged",
      :committer => person,
      :author => person,
      :parents => [@repo.head.target],
      :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")

    commit = @repo.lookup(oid)
    assert_kind_of Time, commit.committer[:time]
  end

  def test_write_commit_without_signature
    name = 'Rugged User'
    email = 'rugged@example.com'
    @repo.config['user.name'] = name
    @repo.config['user.email'] = email

    oid = Rugged::Commit.create(@repo,
      :message => "This is the commit message\n\nThis commit is created from Rugged",
      :parents => [@repo.head.target],
      :tree => "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b")

    commit = @repo.lookup(oid)
    assert_equal name, commit.committer[:name]
    assert_equal email, commit.committer[:email]
    assert_equal name, commit.author[:name]
    assert_equal email, commit.author[:email]
  end

  def test_write_invalid_parents
    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }
    assert_raises TypeError do
      Rugged::Commit.create(@repo,
        :message => "This is the commit message\n\nThis commit is created from Rugged",
        :parents => [:invalid_parent],
        :committer => person,
        :author => person,
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
