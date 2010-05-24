require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Walker stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @walker = Ribbit::Walker.new(@path)
  end

  test "can walk a simple revlist" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    data = []
    4.times do
      data << @walker.next
    end
    shas = data.sort.map {|a| a[0,5] }.join('.')
    assert_equal "4a202.5b5b0.84960.9fd73", shas
    assert_equal false, @walker.next
  end

  test "can walk a part of a revlist" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next
    assert_equal false, @walker.next
  end

  test "can hide part of a list" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    @walker.hide("5b5b025afb0b4c913b4c338a42934a3863bf3644")
    2.times { @walker.next }
    assert_equal false, @walker.next
  end

  test "can reset a walker" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next
    assert_equal false, @walker.next
    @walker.reset
    assert_equal sha, @walker.next
  end

end