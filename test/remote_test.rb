require "test_helper"
require 'net/http'

class RemoteTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_remote_connect
    begin
      Net::HTTP.new('github.com').head('/')
    rescue SocketError => msg
      skip "github is not reachable: #{msg}"
    end

    remote = Rugged::Remote.new(@repo, 'git://github.com/libgit2/libgit2.git')

    remote.connect(:fetch) do |r|
      assert r.connected?
    end

    assert !remote.connected?
  end

  def test_list_remotes
    remotes = @repo.remotes
    assert remotes.kind_of? Enumerable
    assert_equal [ "libgit2" ], remotes.to_a
  end

  def test_remote_new_name
    remote = Rugged::Remote.new(@repo, 'git://github.com/libgit2/libgit2.git')
    assert_nil remote.name
    assert_equal 'git://github.com/libgit2/libgit2.git', remote.url
  end

  def test_remote_new_invalid_url
    assert_raises ArgumentError do
      Rugged::Remote.new(@repo, 'libgit2')
    end
  end

  def test_remote_lookup
    remote = Rugged::Remote.lookup(@repo, 'libgit2')
    assert_equal 'git://github.com/libgit2/libgit2.git', remote.url
    assert_equal 'libgit2', remote.name
  end

  def test_remote_lookup_missing
    assert_nil Rugged::Remote.lookup(@repo, 'missing_remote')
  end

  def test_remote_lookup_invalid
    assert_raises Rugged::ConfigError do
      Rugged::Remote.lookup(@repo, "*\?")
    end
  end
end

class RemotePushTest < Rugged::SandboxedTestCase
  def setup
    super
    @remote_repo = sandbox_init("testrepo.git")
    # We can only push to bare repos
    @remote_repo.config['core.bare'] = 'true'

    @repo = sandbox_clone("testrepo.git", "testrepo")
    Rugged::Reference.create(@repo,
      "refs/heads/unit_test",
      "8496071c1b46c854b31185ea97743be6a8774479")

    @remote = Rugged::Remote.lookup(@repo, 'origin')
  end

  def test_push_single_ref
    result = @remote.push(["refs/heads/master", "refs/heads/master:refs/heads/foobar", "refs/heads/unit_test"])
    assert_equal({}, result)

    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", @remote_repo.ref("refs/heads/foobar").target
    assert_equal "8496071c1b46c854b31185ea97743be6a8774479", @remote_repo.ref("refs/heads/unit_test").target
  end

  def test_push_to_non_bare_raise_error
    @remote_repo.config['core.bare'] = 'false'

    assert_raises Rugged::Error do
      @remote.push(["refs/heads/master"])
    end
  end

  def test_push_non_forward_raise_error
    assert_raises Rugged::Error do
      @remote.push(["refs/heads/unit_test:refs/heads/master"])
    end

    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", @remote_repo.ref("refs/heads/master").target
  end

  def test_push_non_forward_forced_raise_no_error
    result = @remote.push(["+refs/heads/unit_test:refs/heads/master"])
    assert_equal({}, result)

    assert_equal "8496071c1b46c854b31185ea97743be6a8774479", @remote_repo.ref("refs/heads/master").target
  end
end

class RemoteWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_remote_add
    Rugged::Remote.add(@repo, 'upstream', 'git://github.com/libgit2/libgit2.git')
    remote = Rugged::Remote.lookup(@repo, 'upstream')
    assert_equal 'upstream', remote.name
    assert_equal 'git://github.com/libgit2/libgit2.git', remote.url
  end

  def test_remote_add_with_invalid_url
    assert_raises ArgumentError do
      Rugged::Remote.add(@repo, 'upstream', 'libgit2')
    end
  end
end
