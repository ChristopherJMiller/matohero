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

          shellHook = ''
            echo "bionicle-heroes decomp shell — game files live in ./orig (gitignored)"
          '';
        };
      });
    };
}
