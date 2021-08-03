{ lib
, boost
, cmake
, curl
, egil
, nix-filter
, openldap
, stdenv
, doCheck ? true
, isDebugBuild ? false
}:

let
  inherit (lib) optionalString;
  inherit (nix-filter) inDirectory;
  inherit (egil.internal) version meta;

  pname = "egil-scim-client";
  mainProgram = "EgilSCIMClient";
in
stdenv.mkDerivation {
  inherit pname;
  version = "${version}${optionalString isDebugBuild "-debug"}";

  src = nix-filter {
    root = ./../..;
    include = [
      "CMakeLists.txt"
      (inDirectory "src")
    ];
    name = pname;
  };

  outputs = [ "bin" "dev" "out" ];
  propagatedBuildOutputs = [ ];

  strictDeps = true;

  buildInputs = [
    boost.dev
    curl.dev # libcurl
    openldap.dev # libldap
  ];

  nativeBuildInputs = [
    cmake
  ];

  dontPatch = true;

  cmakeFlags = [
    (optionalString isDebugBuild "-DCMAKE_BUILD_TYPE=Debug")
  ];

  inherit doCheck;
  checkPhase = ''
    ./tests
  '';

  installPhase = ''
    mkdir -p $bin/bin/
    cp ${mainProgram} $bin/bin/${mainProgram}

    mkdir -p $dev/include/
    cp ../src/pp_interface.h $dev/include/

    mkdir $out
  '';

  dontStrip = isDebugBuild;

  doInstallCheck = true;
  installCheckPhase = ''
    $bin/bin/${mainProgram} --version
  '';

  passthru = {
    debug = egil.scim-client.override { isDebugBuild = true; };
  };

  meta = meta // {
    description = "The EGIL SCIM client";
    longDescription = ''
      The EGIL SCIM client implements the EGIL profile of the SS 12000
      standard. It reads information about students, groups etc. from LDAP and
      sends updates to a SCIM server.
    '';

    license = lib.licenses.agpl3Plus;
    inherit mainProgram;
  };
}
