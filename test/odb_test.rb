require "test_helper"

class OdbTest < Rugged::TestCase
  def setup
    @repo = FixtureRepo.from_rugged("testrepo.git")
  end

  def test_new
    odb = Rugged::Odb.new()
    assert_instance_of Rugged::Odb, odb
  end

  def test_add_backend
    odb = Rugged::Odb.new()
    odb.add_backend(Rugged::Odb::Backend::Loose.new(File.join(@repo.path, "objects"), -1, 0, 0, 0), 1)
  end

  def test_each_loose
    odb = Rugged::Odb.new()
    odb.add_backend(Rugged::Odb::Backend::Loose.new(File.join(@repo.path, "objects"), -1, 0, 0, 0), 1)

    ids = []
    odb.each { |id| ids << id }
    assert_equal 31, ids.length
  end

  def test_each_pack
    odb = Rugged::Odb.new()
    odb.add_backend(Rugged::Odb::Backend::Pack.new(File.join(@repo.path, "objects")), 1)

    ids = []
    odb.each { |id| ids << id }
    assert_equal 6, ids.length
  end

  def test_each_one_pack
    odb = Rugged::Odb.new()
    odb.add_backend(Rugged::Odb::Backend::OnePack.new(File.join(@repo.path, "objects", "pack", "pack-d7c6adf9f61318f041845b01440d09aa7a91e1b5.idx")), 1)

    ids = []
    odb.each { |id| ids << id }
    assert_equal 6, ids.length
  end
end
