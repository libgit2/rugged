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

    remote = Rugged::Remote.new(@repo, "git://github.com/libgit2/libgit2.git")

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
end
