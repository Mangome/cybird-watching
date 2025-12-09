@echo off
echo ======================================
echo    Font Character Extraction Tool
echo ======================================
echo.

cd /d "%~dp0uniq_fonts"
python main.py

echo.
echo ======================================
echo Press any key to exit...
pause >nul
