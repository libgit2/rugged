module Rugged
  class Branch < Rugged::Reference
    def tip
      @owner.lookup(self.resolve.target)
    end

    def ==(other)
      other.instance_of?(Rugged::Branch) &&
        other.canonical_name == self.canonical_name
    end

    alias_method 'canonical_name', 'name'

    def name
      super.gsub(%r{^(refs/heads/|refs/remotes/)}, '')
    end
  end
end
