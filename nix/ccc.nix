# chaoticgd/ccc: parses PS2 .mdebug/STABS debug info (stdump, uncc). MIT.
{ lib, stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation {
  pname = "ccc";
  version = "2.1";

  src = fetchFromGitHub {
    owner = "chaoticgd";
    repo = "ccc";
    rev = "v2.1";
    hash = "sha256-im1mevU5762JGv4KiC32vxg4jjlqDxQ12vlt9Cx9xng=";
  };

  nativeBuildInputs = [ cmake ];

  # The tools have no install rules. objdump is renamed so it can't shadow binutils.
  buildFlags = [ "stdump" "uncc" "demangle" "objdump" ];
  installPhase = ''
    runHook preInstall
    install -Dm755 stdump uncc demangle -t $out/bin
    install -Dm755 objdump $out/bin/ccc-objdump
    runHook postInstall
  '';

  meta = {
    description = "PS2 debug symbol (.mdebug/STABS) parser and tools";
    homepage = "https://github.com/chaoticgd/ccc";
    license = lib.licenses.mit;
  };
}
