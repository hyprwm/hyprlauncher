{
  lib,
  stdenv,
  cmake,
  pkg-config,
  libdrm,
  libxkbcommon,
  hyprlang,
  hyprutils,
  hyprwire,
  aquamarine,
  cairo,
  pixman,
  hyprtoolkit,
  hyprgraphics,
  libqalculate,
  wayland,
  wayland-protocols,
  wayland-scanner,
  hyprwayland-scanner,
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
  ];

  buildInputs = [
    libdrm
    libxkbcommon
    libqalculate
    hyprlang
    hyprutils
    hyprwire
    hyprtoolkit
    hyprgraphics
    wayland
    wayland-protocols
    wayland-scanner
    aquamarine
    pixman
    cairo
  ];

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
