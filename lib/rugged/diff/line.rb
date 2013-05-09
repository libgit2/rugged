module Rugged
  class Diff
    class Line
      attr_reader :line_origin, :content, :owner, :old_lineno, :new_lineno

      alias hunk owner

      def context?
        @line_origin == :context
      end

      def addition?
        @line_origin == :addition
      end

      def deletion?
        @line_origin == :deletion
      end

      def eof_newline?
        @line_origin == :eof_newline
      end

      def inspect
        "#<#{self.class.name}:#{object_id} {line_origin: #{line_origin.inspect}, content: #{content.inspect}>"
      end
    end
  end
end
