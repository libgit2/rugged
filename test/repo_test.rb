require 'test_helper'
require 'base64'

describe Rugged::Repository do
  before do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)

    @test_content = "my test data\n"
    @test_content_type = 'blob'
  end

  it "last_commit returns the most recent commit" do
    assert @repo.respond_to? :last_commit
    assert "36060c58702ed4c2a40832c51758d5344201d89a", @repo.last_commit.oid
  end

  it "fails to open unexisting repositories" do
    assert_raises IOError do
      Rugged::Repository.new("fakepath/123/")
    end

    assert_raises Rugged::RepositoryError do
      Rugged::Repository.new("/home/")
    end
  end

  it "can tell if an object exists or not" do
    assert @repo.exists?("8496071c1b46c854b31185ea97743be6a8774479")
    assert @repo.exists?("1385f264afb75a56a5bec74243be9b367ba4ca08")
    assert !@repo.exists?("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    assert !@repo.exists?("8496071c1c46c854b31185ea97743be6a8774479")
  end

  it "can read an object from the db" do
    rawobj = @repo.read("8496071c1b46c854b31185ea97743be6a8774479")
    assert_match 'tree 181037049a54a1eb5fab404658a3a250b44335d7', rawobj.data
    assert_equal 172, rawobj.len
    assert_equal :commit, rawobj.type
  end

  it "checks that reading fails on unexisting objects" do
    assert_raises Rugged::OdbError do
      @repo.read("a496071c1b46c854b31185ea97743be6a8774471")
    end
  end

  it "can hash data" do
    oid = Rugged::Repository::hash(@test_content, @test_content_type)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
  end

  it "can write to the db" do
    oid = @repo.write(@test_content, @test_content_type)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
    assert @repo.exists?("76b1b55ab653581d6f2c7230d34098e837197674")

    rm_loose(oid)
  end

  it "can walk in a block" do
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    list = []
    @repo.walk(oid) { |c| list << c }
    assert list.map {|c| c.oid[0,5] }.join('.'), "a4a7d.c4780.9fd73.4a202.5b5b0.84960"
  end

  it "can walk without a block" do
    commits = @repo.walk('a4a7dce85cf63874e984719f4fdd239f5145052f')

    assert commits.kind_of?(Enumerable)
    assert commits.count > 0
  end

  it "can lookup an object" do
    object = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")

    assert object.kind_of?(Rugged::Commit)
  end

  it "can find a ref" do
    ref = @repo.ref('refs/heads/master')

    assert ref.kind_of?(Rugged::Reference)
    assert_equal 'refs/heads/master', ref.name
  end

  it "can return all refs" do
    refs = @repo.refs

    assert_equal 4, refs.length
  end

  it "can return all refs that match" do
    refs = @repo.refs 'refs/heads'

    assert_equal 2, refs.length
  end

  it "can return the names of all refs" do
    refs = @repo.ref_names

    refs.each {|name| assert name.kind_of?(String)}
    assert_equal 4, refs.count
  end

  it "can return all tags" do
    tags = @repo.tags

    assert_equal 2, tags.count
  end

  it "can return all tags that match" do
    tags = @repo.tags 'v0.9'

    assert_equal 1, tags.count
  end

  it "can return all remotes" do
    remotes = @repo.remotes

    assert_equal 1, remotes.count
  end

  it "can lookup head from repo" do
    head = @repo.head
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", head.target
    assert_equal :direct, head.type
  end

  it "can access a file" do
    sha = '36060c58702ed4c2a40832c51758d5344201d89a'
    content = @repo.file_at(sha, 'new.txt')
    assert_equal "new file\n", content
  end

  it "can be garbage collected" do
    Rugged::Repository.new(@path)
    ObjectSpace.garbage_collect
  end

  it "can enumerate all objects in the odb" do
    assert_equal 30, @repo.each_id.to_a.length
  end

  it "can load alternates" do
    alt_path = File.dirname(__FILE__) + '/fixtures/alternate/objects'
    repo = Rugged::Repository.new(@path, :alternates => [alt_path])
    assert_equal 33, repo.each_id.to_a.length
    assert repo.read('146ae76773c91e3b1d00cf7a338ec55ae58297e2')
  end
end
