module Rugged
  module Credentials
    # A plain-text username and password credential object.
    class Plaintext
      def initialize(options)
        @username, @password = options[:username], options[:password]
      end
    end

    # A ssh key credential object that can optionally be passphrase-protected
    class SshKey
      def initialize(options)
        @username, @publickey, @privatekey, @passphrase = options[:username], options[:publickey], options[:privatekey], options[:passphrase]
      end
    end

    # A "default" credential usable for Negotiate mechanisms like NTLM or
    # Kerberos authentication
    class Default
    end
  end
end
