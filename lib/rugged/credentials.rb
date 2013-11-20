module Rugged
  module Credentials
    # A plain-text username and password credential object.
    class Plaintext
      def initialize(username, password)
        @username, @password = username, password
      end
    end

    # A ssh key credential object that can optionally be passphrase-protected
    class SshKey
      def initialize(username, publickey, privatekey, passphrase)
        @username, @publickey, @privatekey, @passphrase = username, publickey, privatekey, passphrase
      end
    end

    # A "default" credential usable for Negotiate mechanisms like NTLM or
    # Kerberos authentication
    class Default
    end
  end
end
