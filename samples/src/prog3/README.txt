
Information about "prog3" 

This is a solution to programming assignment 3 from CS6965: Parallel
Hardware Ray Tracing taught in F2011 at the University of Utah. See
the class web page for more details: 

http://www.eng.utah.edu/~cs6965/

Programming assignment 3 builds on programming assignment 2 and adds
triangles and boxes to the data types, and also reads the object data
out of TRaX global memory instead of encoding it in the program. It
renders the "cornell box" scene from the scene database by default
(set in the Makefile). The programming assignment is found in this
directory as prog03.pdf.

This program uses the same basic Point3D, Vector3D, Normal, Ray,
ColorRGB, Image, HitRecord, PinholeCamera, and PointLight classes as
prog1 and Prog2. See those directorys' README.txt for more details
about these data types.  It adds, or modifys the following classes:

Box(.cc, .h): A Box class that includes two corners of the box, the
minpt (Point3D), and maxot (Point3D). Also, the address (ID) of the
box, and some placeholders for later when the box will be part of a
hierarchical structure (child and num_prim). Includes an "intersect"
function which intersects a ray with the box and updates a HitRecord
accordingly. Also, a function that computes the normal at the hit
point on the box. 

Tri(.cc, .h): A triangle class that includes three points of the
triangle (p1, p2, and p3, each a Point3D), the ID of the triangle, and
pointers to the material and the address of the triangle in
memory. Also, this derives the e1 and e2 edges needed for intersection
calculation. The intersects function intersects the triangle with a
ray and updates the HitRecord accordingly, and the normal function
returns the Normal at the hit point. 

Scene(.cc, .h): The Scene class now includes all the scene information
as pointers into TRaX global memory - the start of the scene data, the
camera data, the light data, the material data, and the triangle
data. The "render" function loops through all the triangles in the
scene (no BVH yet) and uses the normals to shade the triangles and
render the scene. 

prog3.cc: Just sets things up and calls myScene.render to ray trace
the image. 



