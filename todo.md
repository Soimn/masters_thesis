- decide on a reasonable default for max number of virtual cameras
- Make sure MediaStream__Start and MediaStream__Stop work when double starting/stopping
- Consider srgb framebuffer

- how many sockets should be used?
- when should the connection block?
- make sure socket communication doesn't block sampling from other cameras

- camera shutsdown when stopped
- larger latency compared to direct stream from webcam (probably framerate difference, or not)
- camera doesn't work on discord
- python bindings
