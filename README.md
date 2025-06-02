# Build Instructions
To build the camera requires the MSVC compiler and the Windows 11 SDK, which are provided by Visual Studio ([https://visualstudio.microsoft.com/](https://visualstudio.microsoft.com/)) or alternatively the CLI version Visual Studio Build Tools ([https://aka.ms/vs/17/release/vs_BuildTools.exe](https://aka.ms/vs/17/release/vs_BuildTools.exe)).
Building the camera is then done by using the "x64 Native Tools Command Prompt for VS <year>" (replace year with the visual studio version) to run the `build.bat` script with either "debug" or "release" as parameter.
This script can be found in the holocam repository root directory. Running this build script will compile both the virtual camera and a usage example. The resulting binaries are found in the `build` directory.
Due to the windows frame server locking the dll after running the camera, the machine has to be rebooted before rebuilding the camera to overwrite the dll successfully.

# Installation Instructions
The first time `holo_cam.dll` is built it needs to be registered on the system, to do this run the command

`regsvr32 build\holo_cam.dll`

from a command prompt with administrator priviliges (assuming the command is ran from the root directory of the repository, replace `build\holo_cam.dll` with the path to `holo_cam.dll` otherwise).

# Python Package Installation
To build the Python package first follow the general build and install instructions above to build and install the virtual camera.
This will also build and copy the required dependencies into the `python_package` directory.
Installing the python package can then be done by navigating to the `python_package` directory and running

`pip install ./holocam`
