### **Overview**

Color Palette Applier is a lightweight C program designed to help you remap the colors in an image based on a custom palette. The program allows you to load an image and a palette file, and then replace the colors in the image with the closest matching colors from the palette. This is especially useful for artists and designers who need to maintain consistent color schemes across multiple assets or images.
### **Key Features**

Color Remapping: Automatically replaces colors in your image with the closest matching colors from a provided palette.
Supports Multiple Image Formats: Compatible with various image formats such as PNG, JPEG, BMP, GIF, and more.
User-Friendly Interface: Simple graphical interface with options to select your input image, palette image, and save the output.
### **Example Image**
![example_image](https://github.com/user-attachments/assets/ed19cca7-e3a1-4764-bfc2-e7fa20ef05c5)
![untitled](https://github.com/user-attachments/assets/5b88dbe1-9f72-4d24-8a7e-d19dadfcfd68)

changed to 31 Color Palette

### **How to Use**

Loading an Image: When you start the program, you will be prompted to select an image file that you wish to remap.
Selecting a Palette: After selecting your image, you'll be prompted to choose a palette image. This image should contain the colors you want to map your original image to. It supports colors appearing double.
Remapping Colors: The program will process the image and replace its colors with the closest matches from the palette.
Saving the Output: Once the colors are remapped, you can preview the result and save it to your desired location.

Also able to Use in CMD with arguments with this format: 

> .\ColorPaletteApplier.exe input.file palette.file output.file

### **Requirements**

SDL2 Library: Is shipped with the current release of the program and is required. (No standalone EXE)
### **Known Issues**

File Format Limitations: Some less common image formats may not be supported depending on the current SDL_image setup.
High CPU load when automating many images to be converted due to no multithreading bening used.
