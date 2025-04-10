import holocam
import numpy as np

import cv2

def main():
    width  = 1920
    height = 1080
    fps    = 60
    port   = 3003

    webcam = holocam.HoloCameraReader(holocam.get_camera_names()[0][1], width, height, fps)

    face_model = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")

    def camera_loop(args):
        effect = args.get("effect", 0)

        if effect == 1:
            frame = webcam.read_frame()

            to_present = frame.reshape((width*height*3))

            to_present[1::3] = 0

            return to_present
        elif effect == 2:
            frame = webcam.read_frame()

            face_coords = face_model.detectMultiScale(frame)

            if len(face_coords) == 0:
                return frame
            else:
                x0, y0, w, h = face_coords[0]
                return cv2.rectangle(frame, (x0, y0), (x0 + w, y0 + h), [0, 255, 0], 3)
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
