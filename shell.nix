{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs =
    with pkgs; [
        SDL2
	gnumake
	gcc
	python3
	bash
    ];
  
  shellHook = ''
    sudo ln -sfT ${pkgs.bash}/bin/bash /bin/bash
    sudo ln -sfT ${pkgs.python3}/bin/python3 /bin/python3
  '';
}
