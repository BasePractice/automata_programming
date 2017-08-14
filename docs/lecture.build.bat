@echo off

set LATEX_PATH=D:\MiKTeX
set PDFLATEX=%LATEX_PATH%\miktex\bin\x64\pdflatex.exe
set LATEX=%LATEX_PATH%\miktex\bin\x64\latex.exe
set DVIPDFM=%LATEX_PATH%\miktex\bin\x64\dvipdfm.exe
set BIBTEX=%LATEX_PATH%\miktex\bin\x64\bibtex.exe
set XELATEX=%LATEX_PATH%\miktex\bin\x64\xelatex.exe
set PYTHON_PATH=C:\Python27
set PYTHON_SCRIPTS=%PYTHON_SCRIPTS%\Scripts

set PATH=%PYTHON_PATH%;%PATH%
set PATH=%PYTHON_SCRIPTS%;%PATH%
set PATH=%CD%;%PATH%

del /F /Q %CD%/lecture_Promela_SPIN.pdf
del /F /Q *.lof && del /F /Q *.aux && del /F /Q *.log && del /F /Q *.toc && del /F /Q *.out && del /F /Q *.bbl && del /F /Q *.blg && del /F /Q *.dvi && del /F /Q *.vrb && del /F /Q *.snm && del /F /Q *.nav

%PDFLATEX% -shell-escape lecture_Promela_SPIN.tex
%BIBTEX% lecture_Promela_SPIN
%PDFLATEX% -shell-escape lecture_Promela_SPIN.tex
%PDFLATEX% -shell-escape lecture_Promela_SPIN.tex

del /F /Q *.lof && del /F /Q *.aux && del /F /Q *.log && del /F /Q *.toc && del /F /Q *.out && del /F /Q *.bbl && del /F /Q *.blg && del /F /Q *.dvi && del /F /Q *.vrb && del /F /Q *.snm && del /F /Q *.nav
