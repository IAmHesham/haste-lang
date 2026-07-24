{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
  nativeBuildInputs = with pkgs; [
    python3

    clang-tools
    gnumake
    gf
  ];

  buildInputs = with pkgs; [
    # glibc
    llvm
  ];

  CC = "clang";
  CXX = "clang++";

  shellHook = ''
    # LLVM_CFLAGS="$(llvm-config --cflags)"
    # LLVM_LDFLAGS="$(llvm-config --ldflags --libs core --system-libs --link-static)"
    # # if you're wondering what are we doing here. we're just replacing
    # # new lines with spaces in these arguments. `llvm-config --ldflags` sometimes returns
    # # arguments grouped in new lines. which just makes sh in make freak out.
    # export LLVM_LDFLAGS="$(echo $LLVM_LDFLAGS | tr '\n' ' ')"
    # export LLVM_CFLAGS="$(echo $LLVM_CFLAGS | tr '\n' ' ')"
  '';
}
