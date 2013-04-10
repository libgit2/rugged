module Rugged
  class Diff
    class Patch
      include Enumerable
      alias each each_hunk

      alias size hunk_count
      alias count hunk_count

      attr_accessor :owner
      alias diff owner

      def inspect
        "#<#{self.class.name}:#{object_id}>"
      end

      def hunks
        each_hunk.to_a
      end
    end
  end
end
