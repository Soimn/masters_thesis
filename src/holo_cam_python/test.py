import holocam
import numpy as np

import threading

class HoloCamPP:
    def __init__(self, name, width, height, fps, port, loop):
        def camera_thread_func():
            c = holocam.HoloCam(name, width, height, fps, port)
            c.start()

            thread = threading.current_thread()
            while getattr(thread, "should_run", False):
                if not(c.present(loop(getattr(thread, "args", {})))): break

        self.camera_thread = threading.Thread(target=camera_thread_func)
        self.camera_thread.should_run = True
        self.camera_thread.args       = {}

    def start(self):
        self.camera_thread.start()

    def stop(self):
        self.camera_thread.should_run = False

    def __getitem__(self, key):
        return self.camera_thread.args[key]

    def __setitem__(self, key, value):
        self.camera_thread.args[key] = value

def main():
    width  = 1920
    height = 1080
    fps    = 60
    port   = 3003

    webcam = holocam.HoloCameraReader(holocam.get_camera_names()[0][1], width, height, fps)

    def camera_loop(args):
        effect = args.get("effect", 0)

        if effect == 1:
            frame = webcam.read_frame()

            to_present = frame.reshape((width*height*3))

            to_present[1::3] = 0

            return to_present
        else:
            return webcam.read_frame()

    cam = HoloCamPP("test", width, height, fps, port, camera_loop)

    cam.start()

    while True:
        a = input(":")

        if a == "q":
            cam.stop()
            break
        else:
            try:
                cam["effect"] = int(a)
            except:
                print("Unexpected input")

if __name__ == "__main__": main()
