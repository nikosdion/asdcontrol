{
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
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
      }
    )
    // {
      modules.asdcontrol =
        {
          config,
          lib,
          pkgs,
          ...
        }:
        let
          cfg = config.programs.asdcontrol;
        in
        {
          options.programs.asdcontrol = {
            enable = lib.mkEnableOption "Enables asdcontrol (brightness control for Apple Monitors)";
          };
          config = lib.mkIf cfg.enable {
            environment.defaultPackages = [ self.packages.${pkgs.system}.default ];
            services.udev.extraRules = ''
              KERNEL=="hiddev*", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="1114", GROUP="users", OWNER="root", MODE="0660"
              KERNEL=="hiddev*", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="9243", GROUP="users", OWNER="root", MODE="0660"
            ''; # Studio Display (1114), Pro Display XDR (9243)
          };
        };
    };
}
