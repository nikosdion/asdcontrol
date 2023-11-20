{
  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }: 
  flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      packages.default = pkgs.stdenv.mkDerivation {
        name = "asdcontrol";
        src = pkgs.lib.cleanSource ./.;
        installPhase = ''
          mkdir -p $out/bin
          mv asdcontrol $out/bin
        '';
      };
    });
}
