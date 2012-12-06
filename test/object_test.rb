require "test_helper"
require 'base64'

describe Rugged::Object do
  before do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  it "cannot lookup a non-existant object" do
    assert_raises Rugged::OdbError do
      @repo.lookup("a496071c1b46c854b31185ea97743be6a8774479")
    end
  end

  it "can lookup an object" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal :commit, obj.type
    assert_equal '8496071c1b46c854b31185ea97743be6a8774479', obj.oid
  end

  it "same looked up objects are the same" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    obj2 = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal obj, obj2
  end

  it "can read raw data from an object" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert obj.read_raw
  end

  it "can lookup an object by revision string" do
    obj = @repo.rev_parse("v1.0")
    assert "0c37a5391bbff43c37f0d0371823a5509eed5b1d", obj.oid
    obj = @repo.rev_parse("v1.0^1")
    assert "8496071c1b46c854b31185ea97743be6a8774479", obj.oid
  end

  it "can lookup just an object's oid by revision string" do
    oid = @repo.rev_parse_oid("v1.0")
    assert "0c37a5391bbff43c37f0d0371823a5509eed5b1d", oid
    @repo.rev_parse_oid("v1.0^1")
    assert "8496071c1b46c854b31185ea97743be6a8774479", oid
  end
end
