module Rugged
  # Note is an interface for adding or ispecting Git object notes
  class Note
    include Enumerable

    # Pretty formatting of a Note.
    #
    # Returns a very pretty String.
    def inspect
      "#<Rugged::Note:#{object_id} {message: #{message.inspect}}>"
    end

  end
end
