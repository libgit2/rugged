module Rugged
  class Diff
    class Hunk
      include Enumerable
      alias each each_line

      attr_reader :line_count, :header, :range, :owner

      alias size line_count
      alias count line_count
      alias delta owner

      def inspect
        "#<#{self.class.name}:#{object_id} {header: #{header.inspect}, range: #{range.inspect}>"
      end

      # Returns an Array containing all lines of the hunk.
      def lines
        each_line.to_a
      end
    end
  end
end
