import holocam

w = 1920
h = 1080
r = holocam.HoloCameraReader(holocam.get_camera_names()[0][1], w, h)

cam = holocam.HoloCam("test camera", w, h, 3003)

while True:
    frame = r.read_frame()
    if not(cam.present(frame)): break
