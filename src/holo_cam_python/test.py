import holocam
import numpy as np

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

    cam = holocam.HoloCamThread("test", width, height, fps, port, camera_loop)

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
