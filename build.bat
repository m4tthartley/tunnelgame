@echo off

set opts=-FC -nologo -Zi -W2 /FS /MT
set code=%cd%
pushd build

set files=%code%\main.c w:\math.c
cl %opts% %files% -Fetunnel.exe user32.lib gdi32.lib opengl32.lib

del game*.pdb

set files=%code%\game.c w:\math.c w:\gfx.c
cl %opts% %files% -Fe_game.dll user32.lib gdi32.lib opengl32.lib /LD /link /PDB:game%random%.pdb /DLL /EXPORT:gamestart /EXPORT:gameloop

popd
