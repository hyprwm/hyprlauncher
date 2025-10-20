{
  lib,
  inputs,
  self,
}:
let
  mkDate =
    longDate:
    (lib.concatStringsSep "-" [
      (builtins.substring 0 4 longDate)
      (builtins.substring 4 2 longDate)
      (builtins.substring 6 2 longDate)
    ]);

  version = lib.removeSuffix "\n" (builtins.readFile ../VERSION);
in
{
  default = inputs.self.overlays.hyprlauncher;

  hyprlauncher = lib.composeManyExtensions [
    inputs.hyprwire.overlays.default
    inputs.hyprlang.overlays.default
    inputs.hyprutils.overlays.default
    inputs.hyprtoolkit.overlays.default
    inputs.aquamarine.overlays.default
    inputs.hyprgraphics.overlays.default
    inputs.hyprwayland-scanner.overlays.default
    (final: prev: {
      hyprlauncher = prev.callPackage ./default.nix {
        stdenv = prev.gcc15Stdenv;
        version =
          version
          + "+date="
          + (mkDate (inputs.self.lastModifiedDate or "19700101"))
          + "_"
          + (inputs.self.shortRev or "dirty");
        inherit (final) hyprlang;
        shortRev = self.sourceInfo.shortRev or "dirty";
      };
    })
  ];
}
