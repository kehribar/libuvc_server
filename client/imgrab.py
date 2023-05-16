# -----------------------------------------------------------------------------
# 
# 
# -----------------------------------------------------------------------------
import sys
import zmq
import time
import pygame
import numpy as np

# ...
WIDTH = int(sys.argv[2])
HEIGHT = int(sys.argv[3])

# Socket to talk to server
context = zmq.Context()
imsock = context.socket(zmq.SUB)
imsock.connect("tcp://%s:8002" % sys.argv[1])
imsock.setsockopt(zmq.SUBSCRIBE, b"")

# ...
pygame.init()
pygame.display.set_caption("imgrab client")
screen = pygame.display.set_mode((WIDTH,HEIGHT), pygame.DOUBLEBUF)
camera_surface = pygame.surface.Surface((WIDTH,HEIGHT), 0, 24).convert()

# ...
prev = time.time()
while True:

  # Receive frame
  dat = imsock.recv()
  dat = np.frombuffer(dat, np.uint8)
  dat = dat[...,::-1].copy() # RGB ---> BGR
  dat = dat.reshape(HEIGHT, WIDTH, 3)
  dat = dat.transpose(1,0,2)

  # Visualise
  pygame.surfarray.blit_array(camera_surface, dat)
  screen.blit(camera_surface, (0, 0))
  pygame.display.flip()

  # Analytics
  now = time.time()
  dt = now - prev
  prev = now 

  # ...
  print("Frame Delta: %.1f ms" % (dt * 1000.0))
