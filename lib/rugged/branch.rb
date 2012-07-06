module Rugged
  class Branch
    def self.find(repo, name)
      if name == "HEAD" || name.match(%r{^(refs/heads/|refs/remotes/)})
        ref = repo.ref(name)
      else
        ref = repo.ref("refs/heads/#{name}") || repo.ref("refs/remotes/#{name}")
      end

      Branch.new(repo, ref, ref.name) if ref
    end

    attr_reader :repository, :canonical_name

    def initialize(repo, reference, name)
      @repository = repo
      @reference = reference
      @canonical_name = name
    end

    def ==(other)
      other.instance_of?(Rugged::Branch) &&
        other.canonical_name == self.canonical_name &&
        other.tip == self.tip
    end

    def name
      @canonical_name.gsub(%r{^(refs/heads/|refs/remotes/)}, '')
    end

    def tip
      ref = @reference.type == :symbolic ? @reference.resolve : @reference
      @repository.lookup(ref.target)
    end
  end
end
