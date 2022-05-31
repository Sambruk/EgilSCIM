{ lib
, bash
, docker
, egil
, callPackage
, makeWrapper
, nix-filter
, openssl
, stdenvNoCC
}:

let
  inherit (lib) makeSearchPath;
  inherit (nix-filter) inDirectory;
  inherit (egil.internal) meta version;

  pname = "egil-test-suite";
  mainProgram = "run_test_suite";

  buildInputs = [
    bash.out
    docker.out
    egil.scim-client.bin
    egil.test.server
    openssl.bin
  ];
in
stdenvNoCC.mkDerivation {
  inherit pname version;
  src = nix-filter {
    root = ./../../../test;
    include = [
      (inDirectory "configs")
      (inDirectory "scenarios")
      (inDirectory "scripts")
      (inDirectory "tests")
    ];
    name = pname;
  };

  strictDeps = true;

  inherit buildInputs;
  nativeBuildInputs = [
    makeWrapper
  ];

  patchPhase = ''
    substituteInPlace ./scripts/${mainProgram} \
      --replace "\''${testroot}/test_server_go/EGILTestServer/" "" \
      --replace "\''${testroot}/../build/" ""
  '';

  dontConfigure = true;
  dontBuild = true;

  installPhase = ''
    cp -r . $out

    mkdir $out/bin/
    ln -s $out/scripts/${mainProgram} $out/bin/${mainProgram}
  '';

  postFixup = ''
    for file in $out/scripts/*; do
      [ -f "$file" ] && [ -x "$file" ] || continue
      wrapProgram "$file" --prefix PATH : "${makeSearchPath "bin" buildInputs}"
    done
  '';

  passthru = {
    vmTest = callPackage ./vm-test.nix { };
  };

  meta = meta // {
    description = "The EGIL test suite";
    longDescription = ''
      The system test suite includes an LDAP server and a way to automatically
      populate it with example students, groups, teachers, school units etc.
      There's also a simple system for running scenarios ("Student X is
      deleted"). This might be of interest also for those that want to test
      their server implementations.
    '';

    license = null;
    platforms = [
      "x86_64-linux"
    ];
    inherit mainProgram;
  };
}
