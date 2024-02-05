@echo #clean_xtra_strip.bat

call #clean

rmdir xtra\SDK\dxsdk /s /Q
rmdir xtra\SDK\SDL2-2.0.8 /s /Q
rmdir xtra\SDK\SDL2-devel-2.0.8-mingw /s /Q
rmdir xtra\SDK\SDL2-devel-2.0.8-VC /s /Q

del xtra\SDK\SDL2-devel-2.28.5-mingw.zip /Q


pause

@echo COMPLETED!


pause
