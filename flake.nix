{
  description = "The EGIL SCIM client";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-21.11";
    flake-utils.url = "github:numtide/flake-utils";
    nix-filter.url = "github:numtide/nix-filter";

    nix-utils = {
      url = "github:ilkecan/nix-utils";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      inherit (builtins) elem;
      inherit (nixpkgs.lib) filterAttrs;

      inherit (inputs.flake-utils.lib)
        defaultSystems
        eachSystem
        flattenTree
      ;

      inherit (inputs.nix-utils.lib)
        createDebugSymbolsSearchPath
        getCmakeVersion
        getUnstableVersion
      ;

      supportedSystems = defaultSystems;
      version =
        if self.sourceInfo ? rev
        then getCmakeVersion ./CMakeLists.txt
        else getUnstableVersion self.lastModifiedDate;

      meta = {
        homepage = "https://www.skolfederation.se/egil-scimclient-esc/";
        downloadPage = "https://github.com/Sambruk/EgilSCIM/releases";
        changelog = "https://raw.githubusercontent.com/Sambruk/EgilSCIM/master/CHANGELOG.md";
        maintainers = with nixpkgs.lib.maintainers; [ ilkecan ];
        platforms = supportedSystems;
      };

      privateOverlay = final: prev:
        let
          inherit (prev) callPackage;
        in
        {
          nix-filter = inputs.nix-filter.lib;
          egil = prev.egil // {
            internal = {
              inherit version meta;
            };
          };
        };
    in
    {
      overlay = import ./nix/overlay.nix {
        inherit privateOverlay;
      };
    } // eachSystem supportedSystems (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [
            inputs.nix-utils.overlay
            self.overlay
          ];
        };

        inherit (pkgs) egil callPackage;
        inherit (pkgs.stdenv) isLinux;

        isSupportedOnCurrentSystem = drv:
          elem system drv.meta.platforms;
      in
      rec {
        checks = packages;

        packages = filterAttrs (_: isSupportedOnCurrentSystem) (flattenTree egil);
        defaultPackage = packages.scim-client;

        hydraJobs = {
          build = checks;

          ${ if isLinux then "vmTest" else null } =
            let
              drv = egil.test.suite;
            in
            {
              ${ if isSupportedOnCurrentSystem drv then drv.pname else null } = drv.vmTest;
            };
        };

        devShell = callPackage ./nix/shell.nix { inherit isSupportedOnCurrentSystem; };
      }
    );
}
