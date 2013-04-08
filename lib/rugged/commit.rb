module Rugged
  class Commit

    def self.prettify_message(msg, strip_comments = true)
      Rugged::prettify_message(msg, strip_comments)
    end

    def inspect
      "#<Rugged::Commit:#{object_id} {message: #{message.inspect}, tree: #{tree.inspect}, parents: #{parent_oids}>"
    end

    def diff(other)
      if !other.is_a?(Rugged::Commit)
        raise ArgumentError, "Rugged::Commit expected, given #{other.class.name}"
      end

      self.tree.diff(other.tree)
    end

    # The time when this commit was made effective. This is the same value
    # as the +:time+ attribute for +commit.committer+.
    #
    # Returns a Time object
    def time
      @time ||= Time.at(self.epoch_time)
    end

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
end
