module Rugged
  class Tree
    include Enumerable

    # Walks the tree but only yields blobs
    def walk_blobs(mode=:postorder)
      self.walk(mode) { |root, e| yield root, e if e[:type] == :blob }
    end

    # Walks the tree but only yields subtrees
    def walk_trees(mode=:postorder)
      self.walk(mode) { |root, e| yield root, e if e[:type] == :tree }
    end

    # Iterate over the blobs in this tree
    def each_blob
      self.each { |e| yield e if e[:type] == :blob }
    end

    # Iterat over the subtrees in this tree
    def each_tree
      self.each { |e| yield e if e[:type] == :tree }
    end
  end
end
