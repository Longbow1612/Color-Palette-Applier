/*
    Copyright (c) 2024 Tim Friedrich

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the “Software”), to use,
    copy, and modify the Software for personal and commercial purposes, subject to
    the following conditions:

    1. The Software may not be redistributed, sublicensed, or sold in any form
       without the explicit written permission of the Copyright Holder.

    2. The above copyright notice and this permission notice shall be included in
       all copies or substantial portions of the Software.

    3. The Software is provided "as is," without warranty of any kind, express or
       implied, including but not limited to the warranties of merchantability,
       fitness for a particular purpose, and noninfringement. In no event shall the
       authors or copyright holders be liable for any claim, damages, or other
       liability, whether in an action of contract, tort, or otherwise, arising from,
       out of, or in connection with the Software or the use or other dealings in the
       Software.
*/

#include <windows.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinyfiledialogs.h"

// Struct to represent a color with RGBA components
typedef struct {
    Uint8 r, g, b, a;
} Color;

// Load palette from an image into an array of Colors
void loadPalette(SDL_Surface* paletteSurface, Color** palette, int* paletteSize) {
    if (paletteSurface == NULL || palette == NULL || paletteSize == NULL) {
        return;
    }

    int width = paletteSurface->w;
    int height = paletteSurface->h;
    *paletteSize = width * height; // Total number of pixels in the palette image

    // Allocate memory for the palette
    *palette = (Color*)malloc((*paletteSize) * sizeof(Color));
    if (*palette == NULL) {
        return;
    }

    Uint32* pixels = (Uint32*)paletteSurface->pixels;
    SDL_PixelFormat* format = paletteSurface->format;

    for (int i = 0; i < *paletteSize; i++) {
        SDL_GetRGBA(pixels[i], format, &((*palette)[i].r), &((*palette)[i].g), &((*palette)[i].b), &((*palette)[i].a));
    }
}


// Find the closest matching color in the palette
int findClosestColor(Color* palette, int paletteSize, Color color) {
    int closestIndex = 0;
    int minDistance = 255 * 255 * 3;

    for (int i = 0; i < paletteSize; i++) {
        int dr = palette[i].r - color.r;
        int dg = palette[i].g - color.g;
        int db = palette[i].b - color.b;
        int distance = dr * dr + dg * dg + db * db;
        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }
    return closestIndex;
}

// Replace image colors with closest palette colors, preserving alpha transparency
void remapImageColors(SDL_Surface* imageSurface, Color* palette, int paletteSize) {
    if (!imageSurface) {
        return;
    }

    if (SDL_MUSTLOCK(imageSurface)) {
        if (SDL_LockSurface(imageSurface) != 0) {
            return;
        }
    }

    int width = imageSurface->w;
    int height = imageSurface->h;
    int bpp = imageSurface->format->BytesPerPixel;
    SDL_PixelFormat* format = imageSurface->format;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Uint8* p = (Uint8*)imageSurface->pixels + y * imageSurface->pitch + x * bpp;
            Uint32 pixel;

            // Extract pixel value based on bytes per pixel
            switch (bpp) {
            case 1:
                pixel = *p;
                break;
            case 2:
                pixel = *(Uint16*)p;
                break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                    pixel = p[0] << 16 | p[1] << 8 | p[2];
                else
                    pixel = p[0] | p[1] << 8 | p[2] << 16;
                break;
            case 4:
                pixel = *(Uint32*)p;
                break;
            default:
                pixel = 0;
            }

            // Map color to closest palette color
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
            Color color = { r, g, b, a };
            int closestIndex = findClosestColor(palette, paletteSize, color);
            Uint32 mappedPixel = SDL_MapRGBA(format, palette[closestIndex].r, palette[closestIndex].g, palette[closestIndex].b, a);

            // Assign mapped color back to image
            switch (bpp) {
            case 1:
                *p = mappedPixel;
                break;
            case 2:
                *(Uint16*)p = mappedPixel;
                break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                    p[0] = (mappedPixel >> 16) & 0xff;
                    p[1] = (mappedPixel >> 8) & 0xff;
                    p[2] = mappedPixel & 0xff;
                }
                else {
                    p[0] = mappedPixel & 0xff;
                    p[1] = (mappedPixel >> 8) & 0xff;
                    p[2] = (mappedPixel >> 16) & 0xff;
                }
                break;
            case 4:
                *(Uint32*)p = mappedPixel;
                break;
            }
        }
    }

    if (SDL_MUSTLOCK(imageSurface)) {
        SDL_UnlockSurface(imageSurface);
    }
}

// Render a texture with rounded rectangle
void drawRoundedRectImage(SDL_Renderer* renderer, SDL_Texture* roundedRectTexture, SDL_Rect rect) {
    SDL_SetTextureBlendMode(roundedRectTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(roundedRectTexture, 70);

    SDL_Rect destRect = { rect.x, rect.y, rect.w, rect.h };

    SDL_RenderCopy(renderer, roundedRectTexture, NULL, &destRect);
}

// Render the image within the window, maintaining aspect ratio
void renderImage(SDL_Renderer* renderer, SDL_Texture* texture, int windowWidth, int windowHeight) {
    int imgWidth, imgHeight;
    SDL_QueryTexture(texture, NULL, NULL, &imgWidth, &imgHeight);

    float aspectRatio = (float)imgWidth / imgHeight;
    int renderWidth = windowWidth;
    int renderHeight = windowHeight - 60; // Leave space for buttons

    // Adjust render size to fit within window dimensions
    if ((float)windowWidth / windowHeight > aspectRatio) {
        renderWidth = (int)((windowHeight - 60) * aspectRatio);
    }
    else {
        renderHeight = (int)(windowWidth / aspectRatio);
    }

    SDL_Rect renderQuad = { (windowWidth - renderWidth) / 2, (windowHeight - 60 - renderHeight) / 2, renderWidth, renderHeight };
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &renderQuad);

    SDL_RenderPresent(renderer);
}

// Check if a point is inside a rounded rectangle
int isPointInRoundedRect(int x, int y, SDL_Rect rect, int radius) {
    if (x >= rect.x + radius && x <= rect.x + rect.w - radius && y >= rect.y && y <= rect.y + rect.h) {
        return 1;
    }
    if (y >= rect.y + radius && y <= rect.y + rect.h - radius && x >= rect.x && x <= rect.x + rect.w) {
        return 1;
    }

    int dx, dy;

    // Check each rounded corner
    dx = x - (rect.x + radius);
    dy = y - (rect.y + radius);
    if (dx * dx + dy * dy <= radius * radius) return 1;

    dx = x - (rect.x + rect.w - radius);
    dy = y - (rect.y + radius);
    if (dx * dx + dy * dy <= radius * radius) return 1;

    dx = x - (rect.x + radius);
    dy = y - (rect.y + rect.h - radius);
    if (dx * dx + dy * dy <= radius * radius) return 1;

    dx = x - (rect.x + rect.w - radius);
    dy = y - (rect.y + rect.h - radius);
    if (dx * dx + dy * dy <= radius * radius) return 1;

    return 0;
}

// Render Cancel and Save buttons with rounded rectangle and text
void renderButtons(SDL_Renderer* renderer, SDL_Rect cancelButton, SDL_Rect saveButton, SDL_Texture* roundedRectTexture) {
    drawRoundedRectImage(renderer, roundedRectTexture, cancelButton);

    TTF_Font* font = TTF_OpenFont("C:/Users/Dingus/AppData/Local/Microsoft/Windows/Fonts/Basic-Regular.ttf", 16);
    if (!font) {
        return;
    }
    SDL_Color textColor = { 255, 255, 255, 255 };
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, "Cancel", textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_Rect textRect;
    textRect.x = cancelButton.x + (cancelButton.w - textWidth) / 2;
    textRect.y = cancelButton.y + (cancelButton.h - textHeight) / 2;
    textRect.w = textWidth;
    textRect.h = textHeight;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    drawRoundedRectImage(renderer, roundedRectTexture, saveButton);

    textSurface = TTF_RenderText_Blended(font, "Save", textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textWidth = textSurface->w;
    textHeight = textSurface->h;
    textRect.x = saveButton.x + (saveButton.w - textWidth) / 2;
    textRect.y = saveButton.y + (saveButton.h - textHeight) / 2;
    textRect.w = textWidth;
    textRect.h = textHeight;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    TTF_CloseFont(font);

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {


    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        SDL_Quit();
        return 1;
    }

    SDL_Surface* imageSurface = NULL;
    SDL_Surface* paletteSurface = NULL;
    Color* palette = NULL;
    int paletteSize = 0;
    if (argc == 4) {

        if (SDL_Init(0) != 0) {
            return 1;
        }

        // Command line mode: use provided arguments
        const char* inputImagePath = argv[1];
        const char* paletteImagePath = argv[2];
        const char* outputImagePath = argv[3];

        imageSurface = IMG_Load(inputImagePath);
        paletteSurface = IMG_Load(paletteImagePath);

        if (!imageSurface || !paletteSurface) {
            if (imageSurface) SDL_FreeSurface(imageSurface);
            if (paletteSurface) SDL_FreeSurface(paletteSurface);
            return 1;
        }

        loadPalette(paletteSurface, &palette, &paletteSize);
        if (palette == NULL) {
            SDL_FreeSurface(imageSurface);
            SDL_FreeSurface(paletteSurface);
            return 1;
        }

        remapImageColors(imageSurface, palette, paletteSize);

        if (IMG_SavePNG(imageSurface, outputImagePath) != 0) {
            return 1;
        }

        free(palette);
        SDL_FreeSurface(imageSurface);
        SDL_FreeSurface(paletteSurface);

    }
    else {

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            return 1;
        }

        if (TTF_Init() == -1) {
            IMG_Quit();
            SDL_Quit();
            return 1;
        }

        // Create SDL window
        SDL_Window* window = SDL_CreateWindow("Image Palette Remapper",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);

        if (!window) {
            IMG_Quit();
            SDL_Quit();
            return 1;
        }

        // Create renderer with VSync enabled
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }

        int quit = 0;
        SDL_Event e;
        SDL_Texture* texture = NULL;

        while (!quit) {


            // Prompt user to select input image
            const char* fileTypes[] = {
            "*.bmp", "*.gif", "*.jpeg", "*.jpg", "*.lbm", "*.pcx", "*.png",
            "*.pnm", "*.ppm", "*.pgm", "*.pbm", "*.qoi", "*.tga", "*.xcf",
            "*.xpm", "*.svg", "*.avif", "*.jxl", "*.tiff", "*.tif", "*.webp"
            };

            const char* inputImagePath = tinyfd_openFileDialog(
                "Select Input Image",        // Dialog title
                ".\\",                       // Default path
                21,                          // Number of file filters
                fileTypes,                   // File type filters
                "Supported Image files",     // File type description
                0                            // Single file selection
            );

            if (quit || !inputImagePath) {
                quit = 1;
                break;
            }
            imageSurface = IMG_Load(inputImagePath);

            // Prompt user to select palette image
            const char* paletteImagePath = tinyfd_openFileDialog(
                "Select Palette Image",      // Dialog title
                ".\\",                       // Default path
                21,                          // Number of file filters
                fileTypes,                   // File type filters
                "Supported Image files",     // File type description
                0                            // Single file selection
            );

            if (!paletteImagePath) {
                SDL_FreeSurface(imageSurface);
                continue;
            }

            paletteSurface = IMG_Load(paletteImagePath);

            if (!imageSurface || !paletteSurface) {
                if (imageSurface) SDL_FreeSurface(imageSurface);
                if (paletteSurface) SDL_FreeSurface(paletteSurface);
                continue;
            }

            loadPalette(paletteSurface, &palette, &paletteSize);
            if (palette == NULL) {
                SDL_FreeSurface(imageSurface);
                SDL_FreeSurface(paletteSurface);
                continue;
            }

            remapImageColors(imageSurface, palette, paletteSize);

            if (texture) {
                SDL_DestroyTexture(texture);
            }
            texture = SDL_CreateTextureFromSurface(renderer, imageSurface);

            int windowWidth, windowHeight;
            SDL_GetWindowSize(window, &windowWidth, &windowHeight);

            // Define button positions
            int buttonWidth = 120;
            int buttonHeight = 100;
            int spacing = 20;
            int margin = 20;

            SDL_Rect cancelButton = { (windowWidth - 2 * buttonWidth - spacing) / 2, windowHeight - buttonHeight - margin, buttonWidth, buttonHeight };
            SDL_Rect saveButton = { cancelButton.x + buttonWidth + spacing, windowHeight - buttonHeight - margin, buttonWidth, buttonHeight };

            SDL_Texture* roundedRectTexture = IMG_LoadTexture(renderer, "./button_background.png");

            if (!roundedRectTexture) {
                free(palette);
                SDL_FreeSurface(imageSurface);
                SDL_FreeSurface(paletteSurface);
                return 1;
            }

            renderImage(renderer, texture, windowWidth, windowHeight);
            renderButtons(renderer, cancelButton, saveButton, roundedRectTexture);

            while (SDL_PollEvent(&e) || !quit) {
                if (e.type == SDL_QUIT) {
                    quit = 1;
                    break;
                }
                else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                    cancelButton.x = (windowWidth - 2 * buttonWidth - spacing) / 2;
                    cancelButton.y = windowHeight - buttonHeight - margin;

                    saveButton.x = cancelButton.x + buttonWidth + spacing;
                    saveButton.y = windowHeight - buttonHeight - margin;

                    renderImage(renderer, texture, windowWidth, windowHeight);
                    renderButtons(renderer, cancelButton, saveButton, roundedRectTexture);
                }
                else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x;
                    int y = e.button.y;
                    int radius = 10;

                    if (isPointInRoundedRect(x, y, cancelButton, radius)) {
                        break;
                    }
                    else if (isPointInRoundedRect(x, y, saveButton, radius)) {
                        const char* outputImagePath = tinyfd_saveFileDialog("Save Output Image", "untitled.png", 21, fileTypes, "Supported Image files");
                        if (outputImagePath && IMG_SavePNG(imageSurface, outputImagePath) == 0) {
                            quit = 1;
                        }
                    }
                }
                SDL_Delay(1);
            }
            free(palette);
            SDL_FreeSurface(imageSurface);
            SDL_FreeSurface(paletteSurface);
            texture = NULL;
            SDL_Delay(1);
        }

        if (texture) {
            SDL_DestroyTexture(texture);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
    }
    IMG_Quit();
    SDL_Quit();


    return 10;
}
