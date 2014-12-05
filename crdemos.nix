# A nix file (nixos.org) to load the necessary software
# to run all of the demos present.

let
  pkgs = import <nixpkgs> {};
  stdenv = pkgs.stdenv;
in rec {
  crdEnv = pkgs.myEnvFun {
    name = "crdemos-env";
    buildInputs = with pkgs; [ gcc perl python ];
    # (for now we assume a dmtcp in the user environment, until
    # dmtcp 2.x is available in nixpkgs)
  };
}