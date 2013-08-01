module Rugged
  class Submodule
    STATUS_LIST.each do |status_name|
      define_method "#{status_name}?" do
        self.status.include?(status_name)
      end
    end
  end
end
