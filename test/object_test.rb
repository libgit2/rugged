require "test_helper"
require 'base64'

class ObjectTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_fail_to_lookup_inexistant_object
    assert_raises Rugged::OdbError do
      @repo.lookup("a496071c1b46c854b31185ea97743be6a8774479")
    end
  end

  def test_lookup_object
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal :commit, obj.type
    assert_equal '8496071c1b46c854b31185ea97743be6a8774479', obj.oid
  end

  def test_objects_are_the_same
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    obj2 = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal obj, obj2
  end

  def test_read_raw_data
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert obj.read_raw
  end

  def test_lookup_by_rev
    obj = @repo.rev_parse("v1.0")
    assert "0c37a5391bbff43c37f0d0371823a5509eed5b1d", obj.oid
    obj = @repo.rev_parse("v1.0^1")
    assert "8496071c1b46c854b31185ea97743be6a8774479", obj.oid
  end

  def test_lookup_oid_by_rev
    oid = @repo.rev_parse_oid("v1.0")
    assert "0c37a5391bbff43c37f0d0371823a5509eed5b1d", oid
    @repo.rev_parse_oid("v1.0^1")
    assert "8496071c1b46c854b31185ea97743be6a8774479", oid
  end
end
