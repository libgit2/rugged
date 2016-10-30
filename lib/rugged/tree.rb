module Rugged
  class Tree
    def self.diff(repo, _self, other = nil, options = {})
      if _self && !_self.is_a?(Rugged::Tree)
        raise TypeError, "At least a Rugged::Tree object is required for diffing"
      end

      if other.nil?
        if _self.nil?
          raise TypeError, "Need 'old' or 'new' for diffing"
        else
          diff_tree_to_tree repo, _self, options
        end
      else
        if other.is_a?(::String)
          other = Rugged::Object.rev_parse repo, other
        end

        _diff(repo, _self, other, options)
      end
    end

    include Enumerable

    attr_reader :owner
    alias repo owner

    def diff(other = nil, options = nil)
      Tree.diff(repo, self, other, options)
    end

    def inspect
      data = "#<Rugged::Tree:#{object_id} {oid: #{oid}}>\n"
      self.each { |e| data << "  <\"#{e[:name]}\" #{e[:oid]}>\n" }
      data
    end

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
