{
  lib,
  stdenv,
  cmake,
  pkg-config,
  aquamarine,
  hyprlang,
  hyprtoolkit,
  hyprutils,
  hyprwayland-scanner,
  hyprwire,
  libqalculate,
  libxkbcommon,
  wayland,
  wayland-protocols,
  wayland-scanner,
  version ? "git",
  shortRev ? "",
}:
stdenv.mkDerivation {
  pname = "hyprlauncher";
  inherit version;

  src = ../.;

  nativeBuildInputs = [
    cmake
    pkg-config
    hyprwayland-scanner
    hyprwire
  ];

  buildInputs = [
    aquamarine
    libqalculate
    libxkbcommon
    hyprlang
    hyprutils
    hyprwire
    hyprtoolkit
    wayland
    wayland-protocols
    wayland-scanner
  ];

  strictDeps = true;

  cmakeFlags = lib.mapAttrsToList lib.cmakeFeature {
    HYPRLAUNCHER_COMMIT = shortRev;
    HYPRLAUNCHER_VERSION_COMMIT = "";
  };

  meta = {
    homepage = "https://github.com/hyprwm/hyprlauncher";
    description = "A multipurpose and versatile launcher / picker for Hyprland";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux;
    mainProgram = "hyprlauncher";
  };
}
