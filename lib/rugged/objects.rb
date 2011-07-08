module Rugged

  class Object
    def <=>(other)
      self.oid <=> other.oid
    end
  end

  class Tree
    include Enumerable
  end

  class Commit
    def to_hash
      {
        :message => message,
        :committer => committer,
        :author => author,
        :tree => tree,
        :parents => parents,
      }
    end

    def modify(new_args, update_ref=nil)
      args = self.to_hash.merge(new_args)
      Commit.create(args, update_ref)
    end
  end

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
