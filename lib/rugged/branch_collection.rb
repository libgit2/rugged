module Rugged
  class BranchCollection
    include Enumerable

    def initialize(repo)
      @repository = repo
    end

    def each(&block)
      Branch.each(@repository).map { |branch| self[branch] }.each(&block)
    end

    def [](name)
      if name == "HEAD" || name.match(%r{^(refs/heads/|refs/remotes/)})
        build_from_refname(name)
      else
        build_from_refname("refs/heads/#{name}") || build_from_refname("refs/remotes/#{name}")
      end
    end

    def create(name, sha_or_ref)
      case sha_or_ref
      when Rugged::Object
        target = sha_or_ref.oid
      else
        if (ref = Rugged::Reference.lookup(@repository, sha_or_ref))
          target = ref.resolve.target
        else
          target = Rugged::Commit.lookup(@repository, sha_or_ref)
        end
      end

      Branch.create(@repository, name, target)
      self[name]
    end

    def move(name, new_name, force = false)
      Branch.move(@repository, name, new_name, force)
      self[new_name]
    end

    def delete(name, type = nil)
      Branch.delete(@repository, name, type)
    end

    protected

    def build_from_refname(refname)
      if (ref = @repository.ref(refname))
        Branch.new(@repository, ref, refname)
      end
    end
  end
end