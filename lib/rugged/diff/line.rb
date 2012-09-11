module Rugged
  class Diff
    class Line
      attr_accessor :line_origin

      attr_accessor :content

      attr_accessor :owner
      alias hunk owner

      def inspect
        "#<#{self.class.name}:#{object_id} {line_origin: #{line_origin.inspect}, content: #{content.inspect}>"
      end
    end
  end
end
