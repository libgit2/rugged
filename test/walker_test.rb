require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Rugged::Walker stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
    @walker = Rugged::Walker.new(@repo)
  end

  test "can walk a simple revlist" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    data = []
    4.times do
      data << @walker.next.sha
    end
    shas = data.sort.map {|a| a[0,5]}.join('.')
    assert_equal "4a202.5b5b0.84960.9fd73", shas
    assert_equal false, @walker.next
  end

  test "can walk a part of a revlist" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next.sha
    assert_equal false, @walker.next
  end

  test "can hide part of a list" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    @walker.hide("5b5b025afb0b4c913b4c338a42934a3863bf3644")
    2.times { @walker.next }
    assert_equal false, @walker.next
  end

  # resetting a walker emtpies the walking queue
  test "can reset a walker" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(sha)
    assert_equal sha, @walker.next.sha
    assert_equal false, @walker.next
    @walker.reset
    assert_equal false, @walker.next
    @walker.push(sha)
    assert_equal sha, @walker.next.sha
  end

  def revlist_with_sorting(sorting)
    sha = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    @walker.sorting(sorting)
    @walker.push(sha)
    data = []
    6.times do
      data << @walker.next
    end
    shas = data.map {|a| a.sha[0,5] if a }.join('.')
  end

  test "can sort order by date" do
    time = revlist_with_sorting(Rugged::SORT_DATE)
    assert_equal "a4a7d.c4780.9fd73.4a202.5b5b0.84960", time
  end

  test "can sort order by topo" do
    topo = revlist_with_sorting(Rugged::SORT_TOPO)
    assert_equal "a4a7d.c4780.9fd73.4a202.5b5b0.84960", topo
  end

  test "can sort order by date reversed" do
    time = revlist_with_sorting(Rugged::SORT_DATE | Rugged::SORT_REVERSE)
    assert_equal "84960.5b5b0.4a202.9fd73.c4780.a4a7d", time
  end

  test "can sort order by topo reversed" do
    topo_rev = revlist_with_sorting(Rugged::SORT_TOPO | Rugged::SORT_REVERSE)
    assert_equal "84960.5b5b0.4a202.9fd73.c4780.a4a7d", topo_rev
  end

end
