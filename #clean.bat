@echo #clean.bat

rmdir id1 /s /Q

rmdir .vs /s /Q
rmdir build-obj /s /Q
rmdir Debug_SDL2-zircon_qualker_32 /s /Q
rmdir Debug_SDL2-zircon_qualker_less_32 /s /Q
rmdir Debug-zircon_beta-x64 /s /Q
rmdir Debug-zircon_qualker_32 /s /Q
rmdir Release-zircon_beta-x64 /s /Q

rem rmdir xtra\SDK\dxsdk /s /Q
rem rmdir xtra\SDK\SDL2-2.0.8 /s /Q
rem rmdir xtra\SDK\SDL2-devel-2.0.8-mingw /s /Q
rem rmdir xtra\SDK\SDL2-devel-2.0.8-VC /s /Q
rem del xtra\SDK\SDL2-devel-2.28.5-mingw.zip /Q

del darkplaces-sdl /Q
del zircon_beta.exe /Q
del zircon_beta.exp /Q
del zircon_beta.lib /Q
del zircon_beta.pdb /Q

@rem this is a hidden file ...
del /A:H #zircon_32_vs_2008.suo /Q

del zircon_beta_linux_dedicated_server /Q
del zircon_beta_linux_sdl /Q
del objectn-sdl /Q
del objectn-dedicated /Q

del #*.layout /Q
del #*.depend /Q
del #*.user /Q
del #*.suo /Q
del #*.ncb /Q



pause
