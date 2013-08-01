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
    def canonical_name
      super
    end

    # Get the remote the branch belongs to.
    #
    # If the branch is remote returns the remote it belongs to.
    # In case of local branch, it returns the remote of the branch
    # it tracks or nil if there is no tracking branch.
    #
    def remote
      remote_name = self.remote_name
      Rugged::Remote.lookup(@owner, remote_name) if remote_name
    end
  end
end
