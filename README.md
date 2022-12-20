# PJEngine (Graphic Engine using Vulkan SDK to render animations in realtime)
Praxisprojekt (*Beginn: SS2022*)

## Installation Guide - OS Windows - Visual Studio CMake Project

IDE | Project Structure
:-------------------------:|:-------------------------:
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_002.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_001.png)

After cloning this repo there are a couple of other tasks which need to be done before executing this project.
First off, the Vulkan SDK must be installed on your Operating System. The regarding SDK might be downloaded from [here](https://vulkan.lunarg.com/).

Currently this project uses version 1.3 of Vulkan.
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_005.png)

Since the goal is to create a **cross-platform project** with **dynamically linked libraries** most of the required 3rd party libraries aren't included 
in the project. The following table shows which other libraries aside from Vulkan are necessary and how they are included.

static | dynamic | dynamic | dynamic
:-------------------------:|:-------------------------:|:-------------------------:|:-------------------------:
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_008.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_006.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_007.png) | Assimp
[Link](https://github.com/ocornut/imgui) | [Link](https://github.com/g-truc/glm) | [Link](https://github.com/glfw/glfw/releases/tag/3.3.7) | [Link](https://github.com/assimp/assimp)

### Linking of dynamic libraries
ImGui is already coming with this repo, but the other libraries need to be installed somewhere on your device. In the next step you will work with CMake Gui
to create a build file to then install the respective library on your device. After that, you have to create global variables pointing to the 
particular installation path.

#### Installing GLM
After opening CMake's GUI you have to select the folders containing the GLM Repo as the source and decide where you want to store the build.
Next up, press *Configure* and stick with the default options. Now you ought to make some changes to the highlighted values and press *Configure* 
again what result should look like in the below image. Finally, you press *Generate* to create the final build folder containing a **sln file** 
that needs to be opened to install the library.

CMake GUI | Opening **sln file** with Visual Studio
:-------------------------:|:-------------------------:
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_009.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_011.png)

"Where to build the binaries?" => folder with the **sln file** that needs to be opened in Visual Studio to install GLM

"CMAKE_INSTALL_PREFIX" => the actual installation path after installing GLM with Visual Studio

#### Installing GLFW
Just follow the same procedure as ~always~ above.

CMake GUI | Opening **sln file** with Visual Studio
:-------------------------:|:-------------------------:
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_010.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_012.png)

#### Installing Assimp
Just follow the same procedure as explained for GLM.

CMake GUI | Opening **sln file** with Visual Studio
:-------------------------:|:-------------------------:
![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_014.png) | ![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_015.png)

### Create Global Variables
This example displays how to create globale variables on Windows.

WARNING: YOU NEED TO NAME THE VARIABLES EXACTLY LIKE SHOWN IN THE UPCOMING IMAGE!

They hold a path value that is located inside the installation folder of GLM and GLFW.

![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_013.png)

## Starting this project in Visual Studio
After finally completing the entire groundwork for this project, you have to open its folder in Visual Studio. You will find a toplevel CMakeLists.txt.
Open this file and click inside the text. Then press *Ctrl+S* resulting in CMake to be compiled. A successful compiling might look like this output:

![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_003.png)

To start the project you are required to open **app.cpp** in *../Procedural-Generation-with-Vulkan/applications/demo/src* and press the green filled triangle.

![](https://github.com/Paul-Johne/Procedural-Generation-with-Vulkan/blob/master/imagesForGithub/installation_guide_004.png)