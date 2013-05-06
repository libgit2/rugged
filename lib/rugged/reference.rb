module Rugged
  class Reference
    alias_method :canonical_name, :name

    def inspect
      "#<Rugged::Reference:#{object_id} {name: #{name.inspect}, target: #{target.inspect}}>"
    end

  end
end
