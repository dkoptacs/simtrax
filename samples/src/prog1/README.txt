
Information about "prog1" 

This is a solution to programming assignment 1 from CS6965: Parallel
Hardware Ray Tracing taught in F2011 at the University of Utah. See
the class web page for more details: 

http://www.eng.utah.edu/~cs6965/

Programming assignment 1 is a very simple ray caster that uses the
TRaX infrastructure. The programming assignment is found in this
directory as prog01-1.pdf. Note that even though these are spheres,
because there is no shading in the very simple example, they look like
discs in the output file. 

This implementation splits out the data structures (classes) into
separate files. I have also chosen to make separate classes for the 3D
types Vector3D, Point3D, and Normal. These are all three-element data
types, but behave slightly differently because of their mathematical
differences. 

Point3D(.cc, .h): A three-element 3D point named Point3D. The three
elements are floats x, y, and z. Includes methods for point
construction, initialization, adding a point to a Vector3D,
subtracting two Point3Ds and returning a Vector3D, negating,
multiplying, dividing, and assignment. Also functions that return the
distance between two points, and distance squared.

Vector3D(.cc, .h): A 3D vector class named Vector3D.  The three
elements are floats x, y, and z. Includes methods for vector
construction, initialization, conversion from a Normal, conversion
from a Point3D, vector addition, subtraction, negation,
multiplication, division, and assignment. Also functions to return
vector length, length-squared, normalize the vector, and dot and cross
product of two vectors.

Normal(.cc, .h): A 3D vector that acts as a Normal. The three elements
are floats x, y, and z. Includes methods for construction,
initialization, adding, adding a Vector3D and a Normal, and
converstions from Point3D and Vector3D. Also normalization of the
Normal.

Ray(.cc, .h): A Ray is a class that includes and origin (o), and a
direction (d), both floats. 

ColorRGB(.cc, .h): A ColorRGB is a class that includes r, g, and b
components, all floats. It represents a color with the r, g, and b
cpmoponents ranging from 0 to 1.0. Includes methods for adding,
subtracting, multiplying, etc. colors with other colors and with
scalars. 

Sphere(.cc, h): A class that implements a Sphere that includes a
center (a POint3D), and a radius (float). Includes an "intersect"
function that takes a Ray as an argument and returns true if the ray
hits the sphere. 

Image(.cc, h): The Image class includes information about the size of
the screen where the image will be displayed, and the location of the
frame buffer in memory. It includes xres and yres screen dimensions
(ints), and an address (int) of the location of the frame buffer in
TRaX memory (start_fb). Includes a "set" function which sets that
value of a pixel (defined by x and y) to a Color (clamped to the range
[0,1.0] in r, g, and b). 

prog1.cc: The main program. Note that it uses "trax_main" instead of
"main". This lets the system add some housekeeping functions for the
trax simulator behind our back. 


