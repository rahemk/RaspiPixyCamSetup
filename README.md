# RaspiPixyCamSetup

This repository provides tools and scripts for using a PixyCam with Raspberry Pi OS.

---

## Setup
Clone the repository and run:
```
bash setup.sh
```

This script will:

Install all required dependencies
Clone and build the PixyCam library

Once everything is installed, you can use the wrapper `pixy-g++` to compile cpp using the PixyCam. 

---

## Usage

### Callibration

The file `calib_homography.cpp` is used to compute a homography that maps image coordinates from the PixyCam to real-world coordinates. Follow the on-screen instructions to collect calibration points. After completion, a `homography.txt` file will be generated.

### Object Tracking

The file `world_coords.cpp` uses the computed homography to display the real-world position of detected objects in real time. Ensure `homography.txt` is present in the working directory. This file is required anywhere the homography transformation is used.


