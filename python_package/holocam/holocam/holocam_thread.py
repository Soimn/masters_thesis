import threading

class HoloCamThread:
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
