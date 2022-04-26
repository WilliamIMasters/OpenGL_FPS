# 2020-COMP3016-Coursework2

## First Person Caster
![Gif](media/Gifs/walking.gif)

First person caster is an fps game made using opengl with very few libraries for my final year opengl module. The only external libraries used were assimp to import 3D models. You are able to walk around the 3D environment and shoot fireballs at the destructible crude oil barrels.

![Gif](media/Gifs/shooting.gif)




## Controlls
WASD - Move forwards, left, back and right
E - shoot fireball
Mouse - Look around
ESC - quit

## How To build
#### PreBuilt 
Click [here](https://github.com/Plymouth-Comp/2020-comp3016-cw2-wmastersPlym/blob/main/FirstPersonCaster.zip) and press download. Extract the file and run the exe.
#### Compile yourself 
Clone the project and open the visual studio solution (Coursework2.sln). Build the project by going to Build -> Build Solution, or by pressing ctrl + shift + B. Then navigate to the root of the solution directory, then go to x64 -> debug, now copy the Assets and media file to the debug folder. Run the exe and enjoy.

## How it works
3D models are loaded through assimp, and then are stored in a GameObject, along with all their parameters like position, collides with player. When the 3D model is loaded its axis-aligned bounding box (hitbox) by looking through all its indices and stores the positions of the maximum and minimum xyz values.

To begin with a window is created, and the mouse callback is set. Then the shaders are loaded from a file and stored in the variable program. Next all the 3D models are loaded and stored in game objects.
Next the main game loop begins, first delta time is calculated, which is used for movement calculations. Next key inputs are registered. Then all the object movements and collisions are preformed. Before any movement it checks to see if the player is touching any gameobjects that have isCollidableWithPlayer = true, if any are found the player is pushed back out the object. Next the players position updated according to its velocity and then it's capped to be within the environment incase of any bugs with the collision resolution. Then all the object collisions are check and resolved using checkCollision().
After all that all the gameobjects are displayed and the players camera is rendered. First everything on the screen is cleared then the projection and view matrixes are calculated and passed to the shaders. Then for each Model its model matrix is calculated and passed to the shaders, and then calls the function within the mesh object to draw the model to the screen.
Finally the glfw buffer is swapped back to the window creating a loop (the main game loop).


## External dependencies
GLEW
freeglut
glm
assimp

https://youtu.be/SamZkCQoHdk