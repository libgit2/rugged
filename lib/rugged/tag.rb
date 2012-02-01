module Rugged
  class Tag

    def inspect
      "#<Rugged::Reference:#{object_id} {name: #{name.inspect}, message: #{message.inspect}, target: #{target.inspect}>"
    end

    def to_hash
      {
        :message => message,
        :name => name,
        :target => target,
        :tagger => tagger,
      }
    end

    def modify(new_args, force=True)
      args = self.to_hash.merge(new_args)
      Tag.create(args, force)
    end
  end
end
