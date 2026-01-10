@echo off
REM ============================================================
REM Batch script to run a Python script on all *.bin files
REM in a specified input directory, saving results as *.txt
REM in a specified output directory.
REM ============================================================

REM === CONFIGURATION ===
set "INPUT_DIR=c:\users\conra\Downloads\silversat-packets_10Jan2026-004500z"
set "OUTPUT_DIR=c:\users\conra\onedrive\documents\parsed_output"
set "PYTHON_SCRIPT=C:\Users\conra\OneDrive\Documents\GitHub\Radio_Software\gnuradio\il2p_parser.py"

REM Create output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Loop through all .bin files in the input directory
for %%F in ("%INPUT_DIR%\*.bin") do (
    REM Extract filename without extension
    set "FILENAME=%%~nF"

    REM Enable delayed expansion for variable usage inside loop
    setlocal enabledelayedexpansion

    REM Build input and output file paths
    set "INPUT_FILE=%%F"
    set "OUTPUT_FILE=%OUTPUT_DIR%\!FILENAME!.txt"

    REM Run Python script with arguments
    echo Processing "!INPUT_FILE!" -> "!OUTPUT_FILE!"
    python "%PYTHON_SCRIPT%" -i="!INPUT_FILE!" -o="!OUTPUT_FILE!"

    endlocal
)

echo All files processed.
pause


