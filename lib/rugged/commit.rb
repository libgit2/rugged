module Rugged
  class Commit
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
