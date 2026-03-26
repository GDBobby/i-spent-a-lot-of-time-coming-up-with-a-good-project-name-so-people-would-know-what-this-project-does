#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

#include "ImageFormat.h"
#include "BMP.h"

namespace ImageProcessor{

    struct RawImage{

        //width and height are in texels/pixels
        uint32_t width;
        uint32_t height;
        uint32_t depth = 1;

        //uint32_t array_layers;
        //uint32_t mip_levels; //check mipmaps.size()

        UndecipherableImageFormat format;

        std::vector<std::byte> processed_data;
        std::vector<std::span<std::byte const>> mipmaps = {};

        bool ProcessData(std::span<std::byte> preprocessed_data);

        private:
            //adding bmp, tga, ppm support should be trivial

            bool LoadDDS(std::span<std::byte> preprocessed_data) noexcept;
            bool LoadPNG(std::span<std::byte> preprocessed_data) noexcept;
            bool LoadJPG(std::span<std::byte> preprocessed_data);
            bool LoadBMP(std::span<std::byte> preprocessed_data) noexcept;
    };

}