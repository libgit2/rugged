module Rugged
  class Tree
    # Walks the tree but only yields blobs
    def each_blob(mode=:postorder)
      self.walk(mode) { |root, e| yield root, e if e[:type] == :blob }
    end

    # Walks the tree but only yields subtrees
    def each_tree(mode=:postorder)
      self.walk(mode) { |root, e| yield root, e if e[:type] == :tree }
    end
  end
end
