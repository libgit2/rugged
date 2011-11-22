require File.expand_path "../test_helper", __FILE__

context "Rugged::Tag tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  test "test remote connection" do
    remote = Rugged::Remote.new(@repo, "git://github.com/libgit2/libgit2.git")

    remote.connect(:fetch) do |r|
      assert r.connected?
    end

    assert (not remote.connected?)
  end

end
