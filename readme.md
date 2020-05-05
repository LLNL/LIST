# Livermore SEM Image Tools (LIST)

Scanning Electron Microscopy (SEM) images provide a variety of structural and morphological information for the characterization of the nanomaterials in material science and chemistry communities. This code offers automatic recognition and quantitative analysis of SEM images in a high-throughput manner using computer vision and machine learning techniques. The main function of this application is to extract particle size and morphology information of overlapping nanoparticles and core-shell nanostructures in a user friendly interface. The code is written in C++ with QT environment, and has been tested on MacOSX. 



## Getting Started

### Prerequisites for source compilation

- [QT 5 or higher](qt.io)
- [OpenCV 4 or higher](opencv.org)
- [Tesseract (with JPEG library)](https://github.com/tesseract-ocr/tesseract)
- [Persistence1d.hpp](http:://www.csc.kth.se/~weinkauf/notes/persistence1d.html)


### Running the application

The following files are required to run the application:
- EAST detector checkpoint: [frozen_east_text_detection.pb](https://github.com/ZER-0-NE/EAST-Detector-for-text-detection-using-OpenCV/blob/master/frozen_east_text_detection.pb), see [argman/EAST on gitHub](https://github.com/argman/EAST) for more detail
- Tesseract trained data: eng.traineddata, osd.traineddata, snum.traineddata, see [tesseract-ocr on GitHub](https://github.com/tesseract-ocr/tesseract) for more detail

These files can be also found [here](https://drive.google.com/drive/folders/1MfLdESnFcCaTKs58F6ZO749nWR7ZJ7Tb?usp=sharing)



## Deployment

### How to make a DMG installer for MacOSX distribution

1. use this keyword in pro: CONFIG += app_bundle
2. run: macdeployqt LIST.app -dmg (make sure to use macdeploygt from the same QT directory)
3. copy any missing dylib to "LIST.app/Contents/Frameworks" manually. For example, the followings need to be copied manually.   
	libopencv_core.4.1.1.dylib
	libopencv_core.4.1.dylib
	libopencv_core.dylib
	libopencv_dnn.4.1.1.dylib
	libopencv_dnn.4.1.dylib
	libopencv_dnn.dylib
	libopencv_imgcodecs.4.1.1.dylib
	libopencv_imgcodecs.4.1.dylib
	libopencv_imgcodecs.dylib
	libopencv_imgproc.4.1.1.dylib
	libopencv_imgproc.4.1.dylib
	libopencv_imgproc.dylib
	libtesseract.4.dylib
4. copy EAST descriptor and tesseract trained data ("frozen_east_text_detection.pb", "eng.traineddata", "osd.traineddata", "snum.traineddata" in "data") to "LIST.app/Contents/Resources"
5. copy "src/LIST.ini" to "LIST.app/Contents/MacOS"
6. copy sample SEM images to "LIST.app/Samples" (optional)
7. run again: macdeployqt LIST.app -dmg (make sure to use macdeploygt from the same QT directory)

A precompiled DMG file as a MacOSX bundle is provided [here](https://drive.google.com/open?id=1_dPXcsjAlofaR7098_31mNNZ4o1S4cqG). Note that the DMG file contains all required libraries and data files. 



## Contributing

To contribute to LIST, please send us a pull request. When you send your request, make develop 
the destination branch on the repository.
 


## Versioning
0.9



## Authors

LIST was created by Hyojin Kim, hkim@llnl.gov. 

### Other contributors
Special thanks to Jinkyu Han (han10@llnl.gov) for extensive program tests and suggestions.
 
This project was supported by a LLNL's lab directed research and development - strategic initiatives project (Accelerating Feedstock Optimization Using Computer Vision, Machine Learning, and Data Analytic Techniques, PI: T. Yong Han). 



## Citing LIST

If you need to reference LIST in a publication, please cite the following paper:

Hyojin Kim, Jinkyu Han, T. Yong Han, "Machine Vision-Driven Automatic Recognition of Particle Size and Morphology in SEM Images", submitted to Nanoscale, 2020. 



## License
LIST is distributed under the terms of the MIT license. All new contributions must be made under this license.

SPDX-License-Identifier: MIT

LLNL-CODE-806098

