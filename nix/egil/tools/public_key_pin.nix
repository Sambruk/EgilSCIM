{ callPackage
, bash
, egil
, openssl
}:

let
  inherit (egil.internal) meta;
in
callPackage ./default.nix {
  filename = "public_key_pin.sh";

  buildInputs = [
    bash.out
    openssl.bin
  ];

  meta = meta // {
    description =
      "A script for generating a public key pin based on an x509 certificate";
    longDescription =
      "Extracts public key from a x509 certificate and outputs its pin. " +
      "Depends on OpenSSL.";

    license = null;
  };

  wrapProgram = true;
}
