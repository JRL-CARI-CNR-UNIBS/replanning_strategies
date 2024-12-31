![](documentation/logo_blue.png?raw=true)

## Introduction
**OpenMORE** contains a library of sampling-based path replanning algorithms. It develops a framework to manage robot's trajectory execution with online path replanning. It is based on ROS and [MoveIt](https://moveit.github.io/moveit_tutorials/) to get information about the environment and collision checking. Check [this paper](https://ieeexplore.ieee.org/abstract/document/10275365) for more information.

## Status
<h1 align="center">🚧 Update in Progress! 🚧</h1>
<p align="center">
  <img src="https://img.shields.io/badge/Status-Updating-blue?style=for-the-badge&logo=github">
</p>
<p align="center">
    We are currently significantly changing the organisation of the library and updating the documentation. Expect new changes in the coming weeks. Stay tuned!
</p>


## Build & Install
While some `OpenMORE`'s packages are ROS-independent, others require compilation within a ROS workspace (e.g.,[`replanners_managers_lib`](https://github.com/JRL-CARI-CNR-UNIBS/replanners_managers_lib)). This tutorial assumes that `OpenMORE` and all its dependencies are installed within the same workspace.

First, install some utility packages following the steps indicated at [this page](https://github.com/JRL-CARI-CNR-UNIBS/cnr_common). These packages provides functionalities for logging, read/write parameters and loading plugins.

Create your workspace:
```bash
mkdir -p ~/openmore_ws/src
cd ~/openmore_ws
catkin init && catkin config --install
wstool init src
```
Download `OpenMORE` and its dependencies:
```bash
cd ~/openmore_ws
wget https://raw.githubusercontent.com/JRL-CARI-CNR-UNIBS/OpenMORE/master/OpenMORE.rosinstall
wstool merge -t src ./OpenMORE.rosinstall
wstool update -t src
rosdep install --from-paths src --ignore-src -r -y
```
Finally, compile the workspace:
```bash
catkin build -cs
source devel/setup.bash
```
### Docker
A [docker file](https://github.com/JRL-CARI-CNR-UNIBS/OpenMORE/blob/master/dockerfile_OpenMORE) is also available. Open a terminal, move into the folder where you have saved the docker file and run the following command:
```
sudo docker build -f dockerfile_OpenMORE -t openmore .
```
Once completed, run the container:
```
xhost + 

sudo docker run -it --net=host --gpus all \
    --env="NVIDIA_DRIVER_CAPABILITIES=all" \
    --env="DISPLAY" \
    --env="QT_X11_NO_MITSHM=1" \
    --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
    openmore
```
Then, inside the container you can try the library.

## Packages Overview
`OpenMORE` is organized into three core packages, each with a distinct role:

1. **[replanners_lib](https://github.com/JRL-CARI-CNR-UNIBS/replanners_lib)**: Implements various sampling-based path replanning algorithms for dynamic path replanning.

2. **[replanners_managers_lib](https://github.com/JRL-CARI-CNR-UNIBS/replanners_managers_lib)**: Provides a framework for managing and executing path replanning algorithms in real-time during robot trajectory execution.

3. **[trajectories_processors_lib](https://github.com/JRL-CARI-CNR-UNIBS/trajectories_processors_lib)**: Provides and interface to convert paths into trajectories and for trajectory interpolation.

## Tutorials
To get started with OpenMORE, refer to the following resources:

- **[Replanners Tutorials](https://github.com/JRL-CARI-CNR-UNIBS/replanners_lib/blob/master/README.md)**: Learn how to use and implement new path replanners.
- **[Replanner Managers Tutorials](https://github.com/JRL-CARI-CNR-UNIBS/replanners_managers_lib/blob/master/README.md)**: Explore how to create and manage replanner architectures.

Additionally, the **[`openmore_ros_examples`](https://github.com/JRL-CARI-CNR-UNIBS/openmore_ros_examples)** repository provides practical examples of using `OpenMORE` exploiting MoveIt for managing the planning scene.

## Work in progress
This repository is a work in progress and is continuously evolving. As such, it is not free of bugs.
 **Please be careful if you use it on real hardware and ensure all necessary safety measures are in place**.

If you find errors or if you have some suggestions, [please let us know](https://github.com/JRL-CARI-CNR-UNIBS/OpenMORE/issues).

We are actively seeking support for further development. If you're interested, please reach out via email at <mailto::c.tonola001@unibs.it>.

Future works:
1. Compatibility with ROS2

## How to cite
Plain text:
```
C. Tonola, M. Beschi, M. Faroni and N. Pedrocchi, "OpenMORE: an open-source tool for sampling-based path replanning in ROS," 2023 IEEE 28th International Conference on Emerging Technologies and Factory Automation (ETFA), Sinaia, Romania, 2023, pp. 1-4, doi: 10.1109/ETFA54631.2023.10275365.
```

BibTex:
```
@INPROCEEDINGS{openmore,
  author={Tonola, Cesare and Beschi, Manuel and Faroni, Marco and Pedrocchi, Nicola},
  booktitle={2023 IEEE 28th International Conference on Emerging Technologies and Factory Automation (ETFA)}, 
  title={{OpenMORE: an open-source tool for sampling-based path replanning in ROS}}, 
  year={2023},
  volume={},
  number={},
  pages={1-4},
  doi={10.1109/ETFA54631.2023.10275365}} 
```

## Developer Contact
### **Authors**
- Cesare Tonola (<mailto::c.tonola001@unibs.it>)
- Manuel Beschi (<mailto::manuel.beschi@unibs.it>)

## Acknowledgements
**OpenMORE** is developed with [CNR-STIIMA](http://www.stiima.cnr.it/) and [University of Brescia](https://www.unibs.it/en).

***

![EC-H2020](documentation/Sharework.png) [ShareWork webpage](https://sharework-project.eu/)

![EC-H2020](documentation/flag_yellow.jpg)

This project has received funding from the European Union’s Horizon 2020 research and innovation programme under grant agreement No. 820807.
This website reflects only the author’s view and the European Commission is not responsible for any use that may be made of the information it contains.
