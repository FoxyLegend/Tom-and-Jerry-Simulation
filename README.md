# Tom-and-Jerry Simulation ([Transmitter Hunting](https://en.wikipedia.org/wiki/Transmitter_hunting))
Implementation of virtual gears required for the sport called the Foxhunt game where ‘hounds’ hunt down ‘a fox’ by observing the virtual signal calculated from a signal simulation using real GPS and Map data using mobile smartphones and multi-core processing server. We renamed to Tom-and-Jerry game for intuitive understanding of this game.

## Presentation
* **MOSP**: click to play the video on YouTube.

[![Middle of Semester Presentation](https://img.youtube.com/vi/RQ_oRsLWNQ4/0.jpg)](https://www.youtube.com/watch?v=RQ_oRsLWNQ4 "Middle of Semester Presentation")

## Introduction
![alt tag](README/introduction.jpg)

## Documents
* [2017/03/20] - [Idea pitching]([2017.3.20]_idea_pitching.pdf)
* [2017/04/05] - [Project document]([2017.4.05]_project_document.pdf)
* [2017/04/10] - [MOSP (Interim Presentation)]([2017.4.10]_middle_of_semester_presentation.pdf)

## Installation
### OSM crawling

1. Install [BeautifulSoup](https://www.crummy.com/software/BeautifulSoup/bs4/doc/#installing-beautiful-soup).

        $ pip install beautifulsoup4

2. Export [OpenStreetMap](http://www.openstreetmap.org/export) data that will be used.
3. Run our [Python Code](OSMcrawling/MapData_Crawling.py).
4. Then you can get the results: (e.g.)

	   N1
	   2325103531 36.3743666 127.3651685
	   2325103532 36.3740834 127.3651743
	   2325103536 36.3740856 127.3653625
	   2325103535 36.3740132 127.3653624
	   2325103534 36.3740139 127.3662224
	   2325103533 36.3743676 127.3662207
	   4627616337 36.3743663 127.3656028
	   2325103531 36.3743666 127.3651685

### Signal Simulation
**Simulation on: # of signal:360**

[![Signal Simulation](https://img.youtube.com/vi/6H-wo_vZulA/0.jpg)](https://www.youtube.com/watch?v=6H-wo_vZulA "Signal Simulation")

I'm looking for some other method to delete inf. loop of reflection.


### [Open MPI](https://www.open-mpi.org/)
Our first proposal included MPI technology, but now we only consider to use only GPU because it's enough.

### OpenCL-[NVIDIA CUDA](https://developer.nvidia.com/cuda-downloads)
OpenCL should be installed based on your graphics processing unit (GPU). (e.g. AMD, NVIDIA) We are using CUDA from NVIDIA in this project.

### [Unity 3D](https://unity3d.com/)
In this project, we are using [reinterpret](https://github.com/reinterpretcat/utymap/)'s demo code. Make sure to install correctly. You can also run this demo code on Android.

![alt tag](README/reinterpret_utymap.png)


