@echo off

bossac.exe -i -d --port=%1 -U true -i -e -w -v "collector.bin" -R
