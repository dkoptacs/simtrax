# file: <path>/simtrax/samples/src/pathtracer/CMakeLists.txt


# Write out all of the sources here
# Note that the paths are relative to directory where this CMakeLists.txt is located
set(PATHTRACER_SRC
	raytrace.cc
)

# Write out all of the headers here
set(PATHTRACER_HDR
	BoundingVolumeHierarchy.h
	Box.h
	Color.h
	HitRecord.h
	Image.h
	LambertianMaterial.h
	LambertianTexturedMaterial.h
	Light.h
	Math.h
	Ray.h
	RayCamera.h
	Scene.h
	Sphere.h
	Texture.h
	Triangle.h
	Vector.h
)

 

# get the project set up
# parameters: pathtracer - basename of the project. Results in pathtracer.exe and pathtracer_rt-llvm.s
#             PATHTRACER_SRC - source files list from above
#             PATHTRACER_HDR - header files list from above
#             the other 2 should just be "" each
applyToTarget(pathtracer PATHTRACER_SRC PATHTRACER_HDR "" "")
