module Rugged
  class Remote
    # Push a list of refspecs to the given remote.
    #
    # refspecs - A list of refspecs that should be pushed to the remote.
    #
    # Returns a hash containing the pushed refspecs as keys and
    # any error messages or +nil+ as values.
    def push(refspecs)
      @owner.push(self, refspecs)
    end
  end
end
