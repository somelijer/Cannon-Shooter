#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned
Texture::LoadImageToTexture(const std::string& filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    std::cout << "Loading texture: " << filePath << std::endl;
    unsigned char* ImageData = stbi_load(filePath.c_str(), &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (!ImageData) {
        std::cerr << "Failed to load texture: " << filePath << " loading default instead" << std::endl;
        return LoadImageToTexture(MISSING_TEXTURE_PATH);
    }

    // NOTE(Jovan): Images should usually flipped vertically as they are loaded "upside-down"
    stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

    // NOTE(Jovan): Checks or "guesses" the loaded image's format
    GLint InternalFormat = -1;
    switch (TextureChannels) {
    case 1: InternalFormat = GL_RED; break;
    case 3: InternalFormat = GL_RGB; break;
    case 4: InternalFormat = GL_RGBA; break;
    default: InternalFormat = GL_RGB; break;
    }

    unsigned Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    // NOTE(Jovan): ImageData is no longer necessary in RAM and can be deallocated
    stbi_image_free(ImageData);
    return Texture;
}

unsigned 
Texture::LoadCubemap(std::vector<std::string>& filePaths) {
    unsigned Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

    int width, height, channels;

    for (GLuint i = 0; i < 6; ++i) {
        // Load image data
        unsigned char* imageData = stbi_load(filePaths[i].c_str(), &width, &height, &channels, 0);

        if (!imageData) {
            std::cerr << "Failed to load cubemap texture face: " << filePaths[i] << std::endl;
            stbi_image_free(imageData); // Free previously allocated data
            continue; // Skip this face and continue with the next
        }

        // Vertical flip if necessary
        stbi__vertical_flip(imageData, width, height, channels);

        // Determine internal format based on the number of channels
        GLint internalFormat = -1;
        switch (channels) {
        case 1: internalFormat = GL_RED; break;
        case 3: internalFormat = GL_RGB; break;
        case 4: internalFormat = GL_RGBA; break;
        default: internalFormat = GL_RGB; break;
        }

        // Specify texture data for each face
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, imageData);

        // Free the image data
        stbi_image_free(imageData);
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return Texture;
}
