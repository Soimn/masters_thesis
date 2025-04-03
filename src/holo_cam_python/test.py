import holocam
import threading
import numpy as np
import cv2

def camera_thread_func():
    width  = 1920
    height = 1080
    fps    = 60
    port   = 3003

    cam = holocam.HoloCam("test camera", width, height, fps, port)
    webcam = holocam.HoloCameraReader(holocam.get_camera_names()[0][1], width, height, fps)

    face_classifier = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascades_frontalface_default.xml")

    thread = threading.current_thread()
    while getattr(thread, "should_run", False):
        frame = webcam.read_frame()

        effect = getattr(thread, "effect", 0)

        if effect == 1:
            to_present = (lambda color: color & 0xFF0000)(frame)
        elif effect == 2
            cvframe = (lambda color: [(color & 0xFF0000) >> 16, (color & 0x00FF00) >> 8, color & 0x0000FF])(frame)
            to_present = frame
        else:
            to_present = frame

        if not(cam.present(to_present)): break

def main():
    camera_thread = threading.Thread(target=camera_thread_func)
    camera_thread.should_run = True
    camera_thread.effect     = 0
    camera_thread.start()

    while True:
        a = input(":")

        if a == "q":
            camera_thread.should_run = False
            break
        elif a == "0":
            camera_thread.effect = 0
        elif a == "1":
            camera_thread.effect = 1
        else:
            print("Unexpected input")

if __name__ == "__main__": main()
