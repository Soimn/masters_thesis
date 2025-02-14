run `build.bat debug` to build both the camera and example

the first time holo_cam.dll is built it needs to be registered on the system, to do this run the command `regsvr32 build\holo_cam.dll` from a command prompt with administrator priviliges

due to the windows frame server locking the dll after running the camera, the machine has to be rebooted to for the build to overwrite the dll successfully
