{ lib
, buildInputs
, egil
, filename
, makeWrapper
, meta
, stdenvNoCC
, wrapProgram ? false
}:

let
  inherit (builtins) replaceStrings split elemAt;
  inherit (lib) optional optionalString makeSearchPath;
  inherit (egil.internal) version;

  toolName = elemAt (split "\\." filename) 0;
  mainProgram = filename;
in
stdenvNoCC.mkDerivation {
  pname = "egil-tools-${toolName}";
  inherit version;

  src = ./../../../tools/${filename};

  outputs = [ "bin" "out" ];
  propagatedBuildOutputs = [ ];

  strictDeps = true;

  inherit buildInputs;
  nativeBuildInputs = optional wrapProgram makeWrapper;

  phases = [ "installPhase" "fixupPhase" ];

  installPhase = ''
    mkdir -p $bin/bin
    cp $src $bin/bin/${mainProgram}

    mkdir $out
  '';

  postFixup = optionalString wrapProgram ''
    wrapProgram $bin/bin/${mainProgram} --prefix PATH : "${makeSearchPath "bin" buildInputs}"
  '';

  meta = meta // { inherit mainProgram; };
}
