import holocam

width  = 1920
height = 1080
fps    = 30
port   = 3003

webcam = holocam.HoloCameraReader(holocam.get_camera_names()[0][1], width, height, fps)

def camera_loop(args):
    frame = webcam.read_frame()

    # the webcam frame can be changed here

    return frame

vcam = holocam.HoloCamThread("example", width, height, fps, port, camera_loop)
vcam.start()
