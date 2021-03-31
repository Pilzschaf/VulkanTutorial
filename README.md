# VulkanTutorial

## Setup
Clone the repository with
```git clone https://github.com/Pilzschaf/VulkanTutorial.git```

Go into the directory with
```cd VulkanTutorial```

Initialize submodules with
```git submodule update --init```

## External dependencies
CMake, Vulkan

## Build
### Windows
Create a build folder.

Next open cmake-gui and select the project root as Source folder and the newly created build directory as the build output.

Then click on configure and select your Visual Studio version. Then click on generate. If cmake is unable to find SDL make sure you initialized the submodules as described above.

Inside your build directory you should find a vulkan_tutorial.sln. Open it with Visual Studio.

Make sure to select vulkan_tutorial as the startup project. It should be in a bold typeface.

### Linux
This project uses cmake. First create a build directory and cd to it

```mkdir build && cd build```

Next configure cmake with

```cmake ..```

You should now be able to build the project by typing

```make```

If you want to compile with multiple cores use the following and replace 8 with your desired core count.

```make -j8```

### Run
Executable files are generated inside the bin directory of the project root.