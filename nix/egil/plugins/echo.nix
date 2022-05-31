{ lib
, egil
, stdenv
}:

let
  inherit (egil.internal) meta version;
in
stdenv.mkDerivation {
  pname = "egil-plugins-echo";
  inherit version;

  src = ./../../../plugins/pp/echo;

  outputs = [ "lib" "out" ];
  propagatedBuildOutputs = [ ];

  strictDeps = true;

  buildInputs = [
    egil.scim-client.dev
  ];

  dontPatch = true;
  dontConfigure = true;

  installPhase = ''
    mkdir -p $lib/lib/
    cp libecho.so $lib/lib/

    mkdir $out
  '';

  meta = meta // {
    description = "An example plugin for EgilSCIM";
    longDescription = ''
      An example plugin which simply copies the input text without making any
      real post processing changes.
    '';
    license = lib.licenses.agpl3Plus;
  };
}
