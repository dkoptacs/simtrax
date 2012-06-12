
Information about "prog2" 

This is a solution to programming assignment 2 from CS6965: Parallel
Hardware Ray Tracing taught in F2011 at the University of Utah. See
the class web page for more details: 

http://www.eng.utah.edu/~cs6965/

Programming assignment 2 builds on programming assignment 1 and adds
trax simulation, and also simple Lambertian shading so the spheres
look like spheres. The programming assignment is found in this
directory as prog02.pdf. 

This program uses the same basic Point3D, Vector3D, Normal, Ray,
ColorRGB, and Image classes as prog1. See that directory's README.txt
for more details about these data types.  It adds, or modifys the
following classes: 

HitRecord(.cc, .h): A HitRecord is a data type that records hit
information for a ray. When a ray is compared to all the objects in
the scene, it needs to remember how close the minimum hit distance is
for all the intersections so far so that it can rememeber which
intersection is the closest. It includes Tmin (float) which is the
current minimum distance seen, ObjId (int) which identifys the object
seen at the Tmin distance, and Hitp (boolean) which is a flag to say
whether there has been any hit yet. 

Sphere(.cc, h): A class that implements a Sphere that includes a
center (a POint3D), and a radius (float). The "intersect" function has
been modifie to take a HitRecord (hr) and update that record based on
the object hit and the distance from the ray to the intersection
point. 

Scene(.cc, .h): The Scene class includes the spheres and materials
that make up the scene being rendered. It also includes the resolution
of the screen, the ambient and background colors, and the Ka and Kd
parameters required by the Lambertian shading. The Scene encodes the
entire scene - there is no BVH or other scene database yet. It also
includes a "render" method which actually does the ray tracing on the
scene. It generates the rays, intersects them with all the objects in
the Scene, and updates the frame buffer with the resulting, shaded,
color. 

PointLight(.cc, h): A point light source that inclueds a position
(point3D) and color (defaults to WHITE). 

PinholeCamera(.cc, .h): This defines the parameters of the camera with
in infinitely small aperature. It has an eye (Point3D), a "lookat"
direction (Point3D), and the "up" direction (Point3D). It also
includes the u_len and aspect parameters required to generate ray
directions from the eye through each pixel on the screen. 

prog2.cc: The main program. Note that it uses "trax_main" instead of
"main". This lets the system add some housekeeping functions for the
trax simulator behind our back. All prog2.cc really does is initialize
the scene, image, and camera, and then call myScene.render to render
the image. 




