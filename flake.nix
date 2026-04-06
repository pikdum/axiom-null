{
  description = "Abstract minimalist bullet hell in C with raylib";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      forAllSystems = f:
        nixpkgs.lib.genAttrs systems (system:
          let
            pkgs = import nixpkgs { inherit system; };
          in
          f pkgs system);
    in
    {
      packages = forAllSystems (pkgs: system: {
        default = pkgs.stdenv.mkDerivation {
          pname = "axiom-null";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [
            pkgs.bear
            pkgs.clang
            pkgs.clang-tools
            pkgs.gnumake
            pkgs.pkg-config
          ];

          buildInputs = [
            pkgs.raylib
          ];

          buildPhase = ''
            make game CC=${pkgs.clang}/bin/clang
          '';

          installPhase = ''
            mkdir -p $out/bin
            install -m755 build/game $out/bin/axiom-null
          '';
        };
      });

      checks = forAllSystems (pkgs: system: {
        build = self.packages.${system}.default;

        unit = pkgs.stdenv.mkDerivation {
          pname = "axiom-null-unit";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [
            pkgs.clang
            pkgs.gnumake
          ];

          doCheck = true;

          buildPhase = ''
            make test CC=${pkgs.clang}/bin/clang
          '';

          checkPhase = ''
            make check CC=${pkgs.clang}/bin/clang
          '';

          installPhase = ''
            mkdir -p $out
            touch $out/passed
          '';
        };

        format = pkgs.stdenv.mkDerivation {
          pname = "axiom-null-format";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [
            pkgs.clang-tools
          ];

          buildPhase = ''
            make fmt-check
          '';

          installPhase = ''
            mkdir -p $out
            touch $out/passed
          '';
        };

        lint = pkgs.stdenv.mkDerivation {
          pname = "axiom-null-lint";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = [
            pkgs.bear
            pkgs.clang
            pkgs.clang-tools
            pkgs.gnumake
            pkgs.pkg-config
          ];

          buildInputs = [
            pkgs.raylib
          ];

          buildPhase = ''
            make lint CC=${pkgs.clang}/bin/clang
          '';

          installPhase = ''
            mkdir -p $out
            touch $out/passed
          '';
        };
      });

      devShells = forAllSystems (pkgs: _: {
        default = pkgs.mkShell {
          packages = [
            pkgs.bear
            pkgs.clang
            pkgs.clang-tools
            pkgs.gdb
            pkgs.gnumake
            pkgs.pkg-config
            pkgs.raylib
          ];
        };
      });
    };
}
