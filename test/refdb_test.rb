require "test_helper"

class RefdbTest < Rugged::TestCase
  def setup
    @repo = FixtureRepo.from_rugged("testrepo.git")
  end

  def test_new
    refdb = Rugged::Refdb.new(@repo)
    assert_instance_of Rugged::Refdb, refdb
  end

  def test_set_backend
    refdb = Rugged::Refdb.new(@repo)
    refdb.backend = Rugged::Refdb::Backend::FileSystem.new(@repo)
  end

  def test_set_backend_reuse_error
    refdb = Rugged::Refdb.new(@repo)
    backend = Rugged::Refdb::Backend::FileSystem.new(@repo)

    refdb.backend = backend
    assert_raises RuntimeError do
      refdb.backend = backend
    end
  end
end
