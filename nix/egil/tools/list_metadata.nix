{ lib
, callPackage
, python3
, egil
, pythonPackages
}:

let
  inherit (egil.internal) meta;
in
callPackage ./default.nix {
  filename = "list_metadata.py";

  buildInputs = [
    (python3.withPackages (pkgs: with pkgs; [
      url-normalize
    ]))
  ];

  meta = meta // {
    description = "List contents in authentication metadata";
    longDescription = "A script for listing entities from the metadata.";

    license = lib.licenses.mit;
  };
}
