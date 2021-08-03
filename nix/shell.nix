{ lib
, egil
, gdb
, isSupportedOnCurrentSystem
, mkShell
, nix-utils
, callPackage
}:

let
  inherit (lib) optional;
  inherit (nix-utils) createDebugSymbolsSearchPath;

  debugInfoSearchPath =
    callPackage createDebugSymbolsSearchPath { } [ egil.scim-client.debug ];
in
mkShell {
  packages = [
    gdb
    egil.scim-client.debug.bin
  ] ++ optional (isSupportedOnCurrentSystem egil.test.suite) egil.test.suite;

  inputsFrom = [
    egil.scim-client.debug
  ];

  shellHook = ''
    export NIX_DEBUG_INFO_DIRS=${debugInfoSearchPath}
    alias gdb='gdb --directory=${egil.scim-client.debug.src}/src'
  '';
}
