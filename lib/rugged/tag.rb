module Rugged
  class Tag
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