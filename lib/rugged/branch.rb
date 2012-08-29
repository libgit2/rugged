module Rugged
  class Branch < Rugged::Reference

    # The object pointed at by the tip of this branch
    def tip
      @owner.lookup(self.resolve.target)
    end

    def ==(other)
      other.instance_of?(Rugged::Branch) &&
        other.canonical_name == self.canonical_name
    end

    # The full name of the branch, as a fully-qualified reference
    # path.
    #
    # This is the same as calling Reference#name for the reference behind
    # the path
    alias_method 'canonical_name', 'name'

    # The name of the branch, without a fully-qualified reference path
    #
    # E.g. 'master' instead of 'refs/heads/master'
    def name
      super.gsub(%r{^(refs/heads/|refs/remotes/)}, '')
    end
  end
end
