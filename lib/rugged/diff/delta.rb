module Rugged
  class Diff
    class Delta
      attr_accessor :owner
      alias diff owner

      alias binary? binary

      def inspect
        "#<#{self.class.name}:#{object_id} {old_file: #{old_file.inspect}, new_file: #{new_file.inspect}, similarity: #{similarity.inspect}, status: #{status.inspect}, binary: #{binary.inspect}>"
      end
    end
  end
end
