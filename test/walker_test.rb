require "test_helper"
require 'base64'

context "Rugged::Walker stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
    @walker = Rugged::Walker.new(@repo)
  end

  test "can walk a simple revlist" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    data = @walker.each.to_a
    oids = data.sort { |a, b| a.oid <=> b.oid }.map {|a| a.oid[0,5]}.join('.')
    assert_equal "4a202.5b5b0.84960.9fd73", oids
  end

  test "can walk a part of a revlist" do
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(oid)
    walk = @walker.each.to_a
    assert_equal oid, walk[0].oid
    assert_equal 1, walk.count
  end

  test "can hide part of a list" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    @walker.hide("5b5b025afb0b4c913b4c338a42934a3863bf3644")
    assert_equal 2, @walker.each.count
  end

  # resetting a walker emtpies the walking queue
  test "can reset a walker" do
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    @walker.push(oid)
    walk = @walker.each.to_a
    assert_equal oid, walk[0].oid
    assert_equal 1, walk.count
    @walker.reset
    walk = @walker.each.to_a
    assert_equal 0, walk.count
  end

  test "can enumerable" do
    @walker.push("9fd738e8f7967c078dceed8190330fc8648ee56a")
    enum = @walker.sort { |a, b| a.oid <=> b.oid }.map { |a| a.oid[0, 4] }.join('.')
    assert_equal "4a20.5b5b.8496.9fd7", enum
  end

  def do_sort(sorting)
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    @walker.sorting(sorting)
    @walker.push(oid)
    @walker.each.to_a
  end

  def revlist_with_sorting(sorting)
    data = do_sort sorting
    data.map {|a| a.oid[0,5] if a }.join('.')
  end

  def is_toposorted(list)
    list.all? do |commit|
      commit.parents.all? { |parent| list.index(commit) < list.index(parent) }
    end
  end

  test "can sort order by date" do
    time = revlist_with_sorting(Rugged::SORT_DATE)
    assert_equal "a4a7d.c4780.9fd73.4a202.5b5b0.84960", time
  end

  test "can sort order by topo" do
    sort_list = do_sort(Rugged::SORT_TOPO)
    assert_equal is_toposorted(sort_list), true
  end

  test "can sort order by date reversed" do
    time = revlist_with_sorting(Rugged::SORT_DATE | Rugged::SORT_REVERSE)
    assert_equal "84960.5b5b0.4a202.9fd73.c4780.a4a7d", time
  end

  test "can sort order by topo reversed" do
    sort_list = do_sort(Rugged::SORT_TOPO | Rugged::SORT_REVERSE).reverse
    assert_equal is_toposorted(sort_list), true
  end

end
