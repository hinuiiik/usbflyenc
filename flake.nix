{
  description = "C17 Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            gcc
            cmake
            gnumake
            gdb
            clang-tools
            libsodium

            meson
            ninja
            pkg-config
            gtk4
            glib
            gtk4-layer-shell
          ];
        };
      });
}
