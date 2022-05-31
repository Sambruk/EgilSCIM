{ lib
, callPackage
, egil
, python3
, pythonPackages
}:

let
  inherit (egil.internal) meta;
in
callPackage ./default.nix {
  filename = "fetch_metadata.py";

  buildInputs = [
    (python3.withPackages (pkgs: with pkgs; [
      python-jose
    ]))
  ];

  meta = meta // {
    description = "Download and verify federated TLS authentication metadata";
    longDescription =
      "The script fetch_metadata.py will both download and verify the " +
      "authentication metadata against a key. The decoded metadata can " +
      "then be used by the EGIL client in order to connect to and " +
      "authenticate a server.";

    license = lib.licenses.mit;
  };
}
