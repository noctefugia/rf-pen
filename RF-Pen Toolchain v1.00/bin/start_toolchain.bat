echo off
del /f core_logo.bin
del /f core_font.bin
del /f eeprom.bin
cls
call generate_font.bat
cls
call generate_logo.bat
cls
eeprom_builder.exe
cls
eeprom_loader.exe