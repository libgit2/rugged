require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Object stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @repo = Ribbit::Repository.new(@path)
  end

  test "cannot lookup a non-existant object" do
    obj = @repo.lookup("a496071c1b46c854b31185ea97743be6a8774479")
    assert_equal nil, obj
  end

  test "can lookup an object" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal 'commit', obj.type
    assert_equal '8496071c1b46c854b31185ea97743be6a8774479', obj.sha
  end

  test "same looked up objects are the same" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    obj2 = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal obj, obj2
  end
  
  test "can read raw data from an object" do
    obj = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert obj.read_raw
  end

end
