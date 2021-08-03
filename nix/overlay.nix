{
  privateOverlay
}:

(final: prev:
  let
    pkgs = final.appendOverlays [
      privateOverlay
    ];

    inherit (pkgs) callPackage;
    inherit (pkgs.lib) recurseIntoAttrs;
    inherit (pkgs.egil.internal) version meta;
  in
  {
    egil = {
      scim-client = callPackage ./egil/scim-client.nix { };

      plugins = recurseIntoAttrs {
        echo = callPackage ./egil/plugins/echo.nix { };
      };

      test = recurseIntoAttrs {
        server = callPackage ./egil/test/server.nix { };
        suite = callPackage ./egil/test/suite.nix { };
      };

      tools = recurseIntoAttrs {
        all = callPackage ./egil/tools/all.nix { };
        fetch_metadata = callPackage ./egil/tools/fetch_metadata.nix { };
        list_metadata = callPackage ./egil/tools/list_metadata.nix { };
        public_key_pin = callPackage ./egil/tools/public_key_pin.nix { };
      };
    };
  })
