module Rugged
  # This file is automatically overwritten during a deployment from Travis.
  Version = VERSION = "#{`git describe --tags --first-parent | tail -c +2 | tr '-' '.'`.strip}.pre" rescue "0.0.0.dev"
end
