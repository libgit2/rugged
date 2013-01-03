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
