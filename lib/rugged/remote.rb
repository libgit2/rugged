# Copyright (C) the Rugged contributors.  All rights reserved.
#
# This file is part of Rugged, distributed under the MIT license.
# For full terms see the included LICENSE file.

module Rugged
  class Remote
    # Try to connect to the +remote+. Useful to simulate
    # <tt>git fetch --dry-run</tt> and <tt>git push --dry-run</tt>.
    #
    # Returns +true+ if connection is successful, +false+ otherwise.
    #
    # +direction+ must be either +:fetch+ or +:push+.
    #
    # The following options can be passed in as options:
    #
    # +credentials+ ::
    #   The credentials to use for the connection. Can be either an instance of
    #   one of the Rugged::Credentials types, or a proc returning one of the
    #   former.
    #   The proc will be called with the +url+, the +username+ from the url (if
    #   applicable) and a list of applicable credential types.
    #
    # :headers ::
    #   Extra HTTP headers to include with the request (only applies to http:// or https:// remotes)
    #
    # :proxy_url ::
    #   The url of an http proxy to use to access the remote repository.
    #
    # Example:
    #  remote = repo.remotes["origin"]
    #  success = remote.check_connection(:fetch)
    #  raise Error("Unable to pull without credentials") unless success
    def check_connection(direction, **kwargs)
      connect(direction, kwargs)
      true
    rescue
      false
    ensure
      disconnect()
    end
  end
end
