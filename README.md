# DXE
# A voxel cone traced GI rendering engine in dx12, currently in progress



# current results


![](sc/small2.PNG)
![](sc/sphere1.PNG)
![](sc/imgg.PNG)

### video demo
![](https://www.youtube.com/watch?v=HrUgsg-X_58)

### area lighting
![](sc/area.PNG)

### Indirect + direct lighting
![](sc/indirect.PNG)
### Direct lighting only
![](sc/direct.PNG)

## some other examples, models thanks to talented artists on sketchfab

![](sc/ww22.PNG)
![](sc/ww23.PNG)


## diffuse light only for now
![](sc/diff2.PNG)

![](sc/snibk.PNG)

![](sc/out.PNG)



### Implemented : 
 - Deferred rendering pipeline
 - Scene voxelization using hardware conservative rasterization & geometry shader
 - Radiance volume injection trough shadow map with compute dispatch thread invocation mapping to the dimension of shadowmap instead of volume
 - Anisotropic radiance volume mip map chain generation with compute shader for alleviating light leaking - a widely known issue for VCT.
 - Voxel cone tracing in a single pass conmbining previously generated geometry buffer as well as anisotropic radiance 3d texture buffer to generate final image
 - area/volume lighting
 - mutiple render layers seperating dynamic geometry and static geometry to further optimize voxelization

### Planned
 - better soft shadow from area lights
 - temporal filtering/spatial reprojection for flicker & noise reduction
 - cone traced reflection or SSR, translucent/transmissive object(subsurface scattering)
 - Clip map/cascaded mip map implementation for VCT
 - other mutiple light types using shadow map: point spot
 - cascaded/variance shadow mapping
 - render graph / more orgnized framework & RHI
 - tiled based lighting
 - DXR accelerated or hybrid GI possibly (far/medium range GI utilize Voxel/SDF, close range GI/shadow/reflection utilize DXR or SSGI, which I believe is the GI lighting tech used in UE5's PS5 demo (lumen)) ? (research)  
 - linear transformed cosine
 - DOF
 - may be experiment with SDF (signed distance field) to replace cone trace, heard they look better


## Gbuffer & deferred shawdow 
![](sc/gb.PNG)
![](sc/sm.PNG)

## Voxel 3d volume texture
![](sc/voxeltex.PNG)

## Voxels visulized in screen space with ray marching
![](sc/svoxel.PNG)

## Radiance Map
![](sc/radiance.PNG)

# Credits
 - [UE5 PS5 demo tech analysis]
 - [Sketchfab : Sea Keep "Lonely Watcher"](https://sketchfab.com/3d-models/sea-keep-lonely-watcher-09a15a0c14cb4accaf060a92bc70413d)
 - [Sketchfab : WW2 Cityscene](https://sketchfab.com/3d-models/ww2-cityscene-carentan-inspired-639dc3d330a940a2b9d7f40542eabdf3)