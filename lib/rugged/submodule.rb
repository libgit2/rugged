module Rugged
  class Submodule
    STATUS_LIST = [:in_head, :in_index, :in_config, :in_workdir,
                   :index_added, :index_deleted, :index_modified,
                   :workdir_uninitialized, :workdir_added, :workdir_deleted,
                   :workdir_modified, :workdir_index_modified,
                   :workdir_workdir_modified, :workdir_untracked ]
    STATUS_LIST.each do |status_name|
      define_method "#{status_name}?" do
        self.status.include?(status_name)
      end
    end

  end
end
