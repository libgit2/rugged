module Rugged
  class Index
    include Enumerable

    def diff(*args)
      options = args.pop if args.last.is_a?(Hash)
      other   = args.shift
      if other.nil?
        diff_index_to_workdir options
      else
        if other.is_a? ::Rugged::Commit
          diff_tree_to_index other.tree, options
        else
          if other.is_a? ::Rugged::Tree
            diff_tree_to_index other, options
          else
            raise TypeError, "A Rugged::Commit or Rugged::Tree instance is required"
          end
        end
      end
    end

    def to_s
      s = "#<Rugged::Index\n"
      self.each do |entry|
        s << "  [#{entry[:stage]}] '#{entry[:path]}'\n"
      end
      s + '>'
    end
  end
end
