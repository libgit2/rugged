module Rugged
  class Reference

    def inspect
      "#<Rugged::Reference:#{object_id} {name: #{name.inspect}, target: #{target.inspect}}>"
    end

  end
end
