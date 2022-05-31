{ lib
, egil
, curl
, symlinkJoin
}:

let
  inherit (builtins) attrValues;
  inherit (lib) filterAttrs unique isDerivation;
  inherit (egil.internal) version meta;

  pname = "egil-tools";
  egilToolDrvs = attrValues (filterAttrs (n: v: n != "all" && isDerivation v) egil.tools);
  license = unique (map (drv: drv.meta.license) egilToolDrvs);
in
symlinkJoin {
  inherit pname version;
  name = "${pname}-${version}";

  paths = egilToolDrvs;

  meta = meta // {
    description = "Tools associated with the EGIL client";
    longDescription = ''
      Tools that may simplify usage of the EGIL client. All tools can be run
      with the -h flag to show a brief description of how to run the tool.
    '';
    inherit license;
  };
}
