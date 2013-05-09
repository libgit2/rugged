module Rugged
  class Diff
    class Delta
      include Enumerable

      attr_reader :owner
      alias diff owner

      alias binary? binary

      def added?
        status == :added
      end

      def deleted?
        status == :deleted
      end

      def modified?
        status == :modified
      end

      def renamed?
        status == :renamed
      end

      def copied?
        status == :copied
      end

      def ignored?
        status == :ignored
      end

      def untracked?
        status == :untracked
      end

      def typechange?
        status == :typechange
      end

      def inspect
        "#<#{self.class.name}:#{object_id} {old_file: #{old_file.inspect}, new_file: #{new_file.inspect}, similarity: #{similarity.inspect}, status: #{status.inspect}, binary: #{binary.inspect}>"
      end
    end
  end
end
