
Information about "prog4" 

This is a solution to programming assignment 4 from CS6965: Parallel
Hardware Ray Tracing taught in F2011 at the University of Utah. See
the class web page for more details: 

http://www.eng.utah.edu/~cs6965/

Programming assignment 4 builds on programming assignment 3 and adds
BVH traversal to the ray tracer. The BVH is compiled from the .obj
scene data and loaded into the TRaX global memory by the
compiler. Prog4 renders the "cornell box" scene from the scene
database by default (set in the Makefile). The programming assignment
is found in this directory as prog04.pdf.

This program uses the same basic Point3D, Vector3D, Normal, Ray,
ColorRGB, Image, HitRecord, PinholeCamera, PointLight, Box, and Tri
classes as prog1 and Prog2. See those directorys' README.txt for more
details about these data types.  It adds, or modifys the following
classes:

myBVH(.cc, .h): Very simple class that include the address of the
start of the bvh in TRaX global memory. The action is in the "walk"
and "walkS" functions. These walk the BVH looking for an intersection
with a primitive. If there is an intersection, the HitRecord is
updated. The walkS fuction is optimized for shadows and returns as
soon as there's a hit between the point and the light. 

Scene(.cc, .h): The Scene class now includes all the scene information
as pointers into TRaX global memory - the start of the scene data, the
camera data, the light data, the material data, and the triangle
data. The "render" function now walks the BVH looking for
intersections instead of looping through all primitives. 

prog4.cc: Just sets things up and calls myScene.render to ray trace
the image. 





