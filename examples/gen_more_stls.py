#!/usr/bin/env python3
"""Generate additional test STL files: sphere, pyramid."""
import struct
import math
import os

def write_binary_stl(filename, triangles):
    with open(filename, 'wb') as f:
        f.write(b'\0' * 80)
        f.write(struct.pack('<I', len(triangles)))
        for normal, v1, v2, v3 in triangles:
            for c in normal: f.write(struct.pack('<f', c))
            for c in v1: f.write(struct.pack('<f', c))
            for c in v2: f.write(struct.pack('<f', c))
            for c in v3: f.write(struct.pack('<f', c))
            f.write(struct.pack('<H', 0))

dir = os.path.dirname(os.path.abspath(__file__))

# Sphere: radius 15mm, 32x16 segments, bottom at z=0
r = 15.0
n_lon = 32
n_lat = 16
triangles = []
for i in range(n_lat):
    lat1 = math.pi * i / n_lat
    lat2 = math.pi * (i+1) / n_lat
    for j in range(n_lon):
        lon1 = 2*math.pi * j / n_lon
        lon2 = 2*math.pi * (j+1) / n_lon
        # 4 vertices of this quad on the sphere (shifted up by r so bottom at z=0)
        def sv(lat, lon):
            return (r*math.sin(lat)*math.cos(lon), r*math.sin(lat)*math.sin(lon), r*math.cos(lat) + r)
        p00 = sv(lat1, lon1)
        p01 = sv(lat1, lon2)
        p10 = sv(lat2, lon1)
        p11 = sv(lat2, lon2)
        n = (0,0,1) # dummy normal
        triangles.append((n, p00, p10, p11))
        triangles.append((n, p00, p11, p01))

write_binary_stl(os.path.join(dir, 'sphere_r15.stl'), triangles)

# Pyramid: base 30x30mm, height 40mm, bottom at z=0
b = 15.0  # half-base
h = 40.0
apex = (0, 0, h)
triangles2 = []
# Base (2 triangles)
triangles2.append(((0,0,-1), (-b,-b,0), (b,-b,0), (b,b,0)))
triangles2.append(((0,0,-1), (-b,-b,0), (b,b,0), (-b,b,0)))
# 4 side faces
corners = [(-b,-b,0), (b,-b,0), (b,b,0), (-b,b,0)]
for i in range(4):
    c1 = corners[i]
    c2 = corners[(i+1)%4]
    n = (0,0,1) # dummy
    triangles2.append((n, c1, c2, apex))

write_binary_stl(os.path.join(dir, 'pyramid_30x40.stl'), triangles2)

print("Generated sphere_r15.stl and pyramid_30x40.stl")
