#!/usr/bin/env python3
"""Generate simple test STL files for angled slicing verification."""
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

# 20mm cube centered at origin XY, bottom at z=0
s = 10.0
triangles = []
triangles.append(((0,0,-1), (-s,-s,0), (s,-s,0), (s,s,0)))
triangles.append(((0,0,-1), (-s,-s,0), (s,s,0), (-s,s,0)))
triangles.append(((0,0,1), (-s,-s,2*s), (s,s,2*s), (s,-s,2*s)))
triangles.append(((0,0,1), (-s,-s,2*s), (-s,s,2*s), (s,s,2*s)))
triangles.append(((0,-1,0), (-s,-s,0), (s,-s,0), (s,-s,2*s)))
triangles.append(((0,-1,0), (-s,-s,0), (s,-s,2*s), (-s,-s,2*s)))
triangles.append(((0,1,0), (-s,s,0), (s,s,2*s), (s,s,0)))
triangles.append(((0,1,0), (-s,s,0), (-s,s,2*s), (s,s,2*s)))
triangles.append(((-1,0,0), (-s,-s,0), (-s,s,2*s), (-s,s,0)))
triangles.append(((-1,0,0), (-s,-s,0), (-s,-s,2*s), (-s,s,2*s)))
triangles.append(((1,0,0), (s,-s,0), (s,s,0), (s,s,2*s)))
triangles.append(((1,0,0), (s,-s,0), (s,s,2*s), (s,-s,2*s)))

dir = os.path.dirname(os.path.abspath(__file__))
write_binary_stl(os.path.join(dir, 'cube_20mm.stl'), triangles)

# 10mm tall cylinder, radius 10mm, 24 segments
r = 10.0
h = 10.0
n = 24
triangles2 = []
for i in range(n):
    a1 = 2*math.pi*i/n
    a2 = 2*math.pi*(i+1)/n
    x1, y1 = r*math.cos(a1), r*math.sin(a1)
    x2, y2 = r*math.cos(a2), r*math.sin(a2)
    triangles2.append(((0,0,-1), (0,0,0), (x2,y2,0), (x1,y1,0)))
    triangles2.append(((0,0,1), (0,0,h), (x1,y1,h), (x2,y2,h)))
    nx = (x1+x2)/2; ny = (y1+y2)/2; nl = math.sqrt(nx*nx+ny*ny)
    nn = (nx/nl, ny/nl, 0)
    triangles2.append((nn, (x1,y1,0), (x2,y2,0), (x2,y2,h)))
    triangles2.append((nn, (x1,y1,0), (x2,y2,h), (x1,y1,h)))

write_binary_stl(os.path.join(dir, 'cylinder_r10_h10.stl'), triangles2)
print("Generated cube_20mm.stl and cylinder_r10_h10.stl")
