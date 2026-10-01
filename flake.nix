{
  description = "BIONICLE Heroes (PC, 2006) decompilation dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      }));
    in {
      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            # Disassembly / decompilation
            ghidra
            rizin
            imhex

            # Binary inspection (llvm-objdump understands PE/COFF natively)
            binutils
            llvmPackages.bintools-unwrapped
            upx

            # Unpacking the retail media
            p7zip
            unshield     # InstallShield .cab
            cabextract
            msitools

            # Running the game / the period MSVC toolchain
            wineWow64Packages.stable  # cached; runs 32-bit PEs via new WoW64 mode

            # Scripting
            (python3.withPackages (ps: with ps; [ pefile capstone lief ]))
          ];

          # analyzeHeadless isn't on PATH; scripts use $GHIDRA_INSTALL_DIR/support/analyzeHeadless
          GHIDRA_INSTALL_DIR = "${pkgs.ghidra}/lib/ghidra";

          # Prebuilt wheels in .venv (reccmp's pydemumble) need libstdc++.
          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [ pkgs.stdenv.cc.cc.lib ];

          shellHook = ''
            # reccmp isn't in nixpkgs: keep it in a venv layered over the nix python.
            # Only from the repo root (where requirements.txt lives).
            if [ -f requirements.txt ] && [ -f flake.nix ]; then
              if [ ! -f .venv/.requirements ] || ! cmp -s requirements.txt .venv/.requirements; then
                python3 -m venv --system-site-packages .venv
                .venv/bin/pip install -q -r requirements.txt && cp requirements.txt .venv/.requirements
              fi
              export PATH="$PWD/.venv/bin:$PATH"
            fi
            echo "bionicle-heroes decomp shell — game files live in ./orig (gitignored)"
          '';
        };
      });
    };
}
