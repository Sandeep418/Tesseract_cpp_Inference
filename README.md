## Installation of Tesseract 
# Install vcpkg if you haven't
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install Tesseract
.\vcpkg install tesseract:x64-windows

# Integrate with Visual Studio
.\vcpkg integrate install

# Project Setup in Visual Studio:

Include Directories: C:\Program Files\Tesseract-OCR\include
Library Directories: C:\Program Files\Tesseract-OCR
Copy DLLs: Copy all DLLs from C:\Program Files\Tesseract-OCR to your output directory
