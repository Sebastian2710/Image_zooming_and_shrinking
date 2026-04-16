# Interactive Image Scaling & Interpolation Viewer

This project is a C++ computer vision application built with OpenCV that explores digital image zooming and shrinking techniques. It was developed to study and compare the visual quality and mathematical complexity of three core scaling algorithms: Nearest Neighbor, Bilinear, and Bicubic interpolation.

The application features a custom-built, 4-quadrant interactive Graphical User Interface (GUI) that allows users to seamlessly zoom into 8-bit grayscale images and compare the output of all three algorithms side-by-side in real-time.

## ✨ Key Features

* **Interactive ROI "Minimap":** Left-clicking anywhere on the processed images calculates a dynamic Region of Interest (ROI) with boundary clamping, instantly updating a red minimap tracker on the original source image.
* **Synchronized 4-Quadrant UI:** View the original unscaled image alongside the three processed outputs simultaneously to easily evaluate the advantages and disadvantages of each method.
* **Nearest Neighbor Interpolation:** A fast, foundational scaling method that relies on coordinate rounding, clearly demonstrating spatial pixelation (aliasing) at high zoom levels.
* **Bilinear Interpolation:** A smoother scaling method that calculates the weighted average of the 4 closest surrounding pixels.
* **Bicubic Interpolation:** A highly advanced, professional-grade scaling algorithm that uses a piecewise cubic polynomial to blend a 4x4 grid (16 pixels), preserving smooth gradients and sharper edges.
* **History Stack:** Right-click functionality utilizes a C++ standard stack to remember your previous zoom states, allowing you to perfectly shrink back out of an image step-by-step.

## 🛠️ Built With
* **C++** * **OpenCV (Open Source Computer Vision Library)** ```
