#pragma once

#include "ImageFormat.h"

#include <span>

#include <string_view>
#include <cstdint>

/*
https://github.com/sol-prog/cpp-bmp-images/blob/master/LICENSE
this particular file's license is overwritten by the above license (GPL3)
*/


namespace ImageProcessor{
    struct BMP {
#pragma pack(push, 1)
        struct FileHeader {
            uint16_t file_type{ 0x4D42 };          // File type always BM which is 0x4D42 (stored as hex uint16_t in little endian)
            uint32_t file_size{ 0 };               // Size of the file (in bytes)
            uint16_t reserved1{ 0 };               // Reserved, always 0
            uint16_t reserved2{ 0 };               // Reserved, always 0
            uint32_t offset_data{ 0 };             // Start position of pixel data (bytes from the beginning of the file)
        };
        struct InfoHeader {
            uint32_t size{ 0 };                      // Size of this header (in bytes)
            int32_t width{ 0 };                      // width of bitmap in pixels
            int32_t height{ 0 };                     // width of bitmap in pixels
                                                    //       (if positive, bottom-up, with origin in lower left corner)
                                                    //       (if negative, top-down, with origin in upper left corner)
            uint16_t planes{ 1 };                    // No. of planes for the target device, this is always 1
            uint16_t bit_count{ 0 };                 // No. of bits per pixel
            uint32_t compression{ 0 };               // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
            uint32_t size_image{ 0 };                // 0 - for uncompressed images
            int32_t x_pixels_per_meter{ 0 };
            int32_t y_pixels_per_meter{ 0 };
            uint32_t colors_used{ 0 };               // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
            uint32_t colors_important{ 0 };          // No. of colors used for displaying the bitmap. If 0 all colors are required
        };
        struct ColorHeader {
            uint32_t red_mask{ 0x00ff0000 };         // Bit mask for the red channel
            uint32_t green_mask{ 0x0000ff00 };       // Bit mask for the green channel
            uint32_t blue_mask{ 0x000000ff };        // Bit mask for the blue channel
            uint32_t alpha_mask{ 0xff000000 };       // Bit mask for the alpha channel
            uint32_t color_space_type{ 0x73524742 }; // Default "sRGB" (0x73524742)
            uint32_t unused[16]{ 0 };            // Unused data for sRGB color space
        };
#pragma pack(pop)
        FileHeader file_header;
        InfoHeader bmp_info_header;
        ColorHeader bmp_color_header;
 
        //this constructor is for writing
        [[nodiscard]] BMP(int32_t width, int32_t height, bool has_alpha = true);
        [[nodiscard]] BMP() noexcept; //this constructor is for reading

        void Write(std::string_view fname, std::span<std::byte> data, UndecipherableImageFormat const& format);
        //void Write(std::ofstream &of, std::span<std::byte> data);

    private:
        uint32_t row_stride{ 0 };


        uint32_t Make_Stride_Aligned(uint32_t align_stride);

        // Check if the pixel data is stored as BGRA and if the color space type is sRGB
        void Check_Color_Header(ColorHeader const& _bmp_color_header);
    };
} //namespace ImageProcessor