#include "BMP.h"

#include <fstream>
#include <vector>
/*
https://github.com/sol-prog/cpp-bmp-images/blob/master/LICENSE
this particular file's license is overwritten by the above license (GPL3)
*/

namespace ImageProcessor {

    BMP::BMP() noexcept
    : file_header{},
        bmp_info_header{},
        bmp_color_header{}
    {

    }


    BMP::BMP(int32_t width, int32_t height, bool has_alpha)
    : file_header{},
        bmp_info_header{},
        bmp_color_header{}
    {

        bmp_info_header.width = width;
        bmp_info_header.height = height;
        if (has_alpha) {
            bmp_info_header.size = sizeof(InfoHeader) + sizeof(ColorHeader);
            file_header.offset_data = sizeof(FileHeader) + sizeof(InfoHeader) + sizeof(ColorHeader);

            bmp_info_header.bit_count = 32;
            bmp_info_header.compression = 0;
            row_stride = width * 4;
            const auto data_size = row_stride * height;
            file_header.file_size = file_header.offset_data + data_size;
        }
        else {
            bmp_info_header.size = sizeof(InfoHeader);
            file_header.offset_data = sizeof(FileHeader) + sizeof(InfoHeader);

            bmp_info_header.bit_count = 24;
            bmp_info_header.compression = 0;
            row_stride = width * 3;

            const auto data_size = row_stride * height;
            uint32_t new_stride = Make_Stride_Aligned(4);
            file_header.file_size = file_header.offset_data + data_size + bmp_info_header.height * (new_stride - row_stride);
        }

    }

    void BMP::Write(std::string_view fname, std::span<std::byte> data, UndecipherableImageFormat const& format) {
        std::ofstream of{ fname.data(), std::ios_base::binary };
        if (!of) throw std::runtime_error("Unable to open the output image file.");

        // 1. Determine dimensions and alignment
        const int32_t abs_height = std::abs(bmp_info_header.height);
        const uint32_t bytes_per_pixel = bmp_info_header.bit_count / 8;
        const uint32_t row_stride = bmp_info_header.width * bytes_per_pixel;
        
        // BMP Rows must be 4-byte aligned
        const uint32_t aligned_stride = (row_stride + 3) & ~3;
        const uint32_t padding_size = aligned_stride - row_stride;
        std::vector<char> padding(padding_size, 0);

        // 2. Handle Swizzle (RGBA -> BGRA)
        // If our source is RGB/RGBA but BMP needs BGR/BGRA, we swap in a temporary buffer 
        // or modify the span if allowed. For this example, we assume we might need a swap.
        std::vector<std::byte> swizzled_data;
        const bool needs_bgr_swap = (format.swizzle[0] == Channel::R && format.swizzle[2] == Channel::B);
        
        const std::byte* src_ptr = data.data();
        if (needs_bgr_swap) {
            swizzled_data.assign(data.begin(), data.end());
            for (size_t i = 0; i < swizzled_data.size(); i += bytes_per_pixel) {
                std::swap(swizzled_data[i], swizzled_data[i + 2]);
            }
            src_ptr = swizzled_data.data();
        }

        // 3. Write Headers
        of.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
        of.write(reinterpret_cast<const char*>(&bmp_info_header), sizeof(bmp_info_header));
        
        // Only write color header if it's a V4/V5 header (indicated by size)
        if (bmp_info_header.size >= 108) {
            of.write(reinterpret_cast<const char*>(&bmp_color_header), sizeof(bmp_color_header));
        }

        // 4. Unified Write Loop
        for (int32_t y = 0; y < abs_height; ++y) {
            // If height > 0, BMP is bottom-up (flip index)
            int32_t read_y = (bmp_info_header.height > 0) ? (abs_height - 1 - y) : y;
            
            const char* row_start = reinterpret_cast<const char*>(src_ptr) + (read_y * row_stride);
            
            of.write(row_start, row_stride);
            if (padding_size > 0) {
                of.write(padding.data(), padding_size);
            }
        }

        /*
        std::ofstream of{ fname.data(), std::ios_base::binary };
        if (of) {
            if (bmp_info_header.bit_count == 32) {
                of.write((const char*)&file_header, sizeof(file_header));
                of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
                if(bmp_info_header.bit_count == 32) {
                    of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
                }
                if(bmp_info_header.height > 0){
                    for (int32_t i = bmp_info_header.height - 1; i >= 0; --i) {
                        const char* row_ptr = reinterpret_cast<const char*>(data.data()) + (i * row_stride);
                        of.write(row_ptr, row_stride);
                    }
                }
                else{
                    of.write((const char*)data.data(), data.size());
                }
            }
            else if (bmp_info_header.bit_count == 24) {
                if (bmp_info_header.width % 4 == 0) {  
                    of.write((const char*)&file_header, sizeof(file_header));
                    of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
                    if(bmp_info_header.bit_count == 32) {
                        of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
                    }
                    if(bmp_info_header.height > 0){
                        for (int32_t i = bmp_info_header.height - 1; i >= 0; --i) {
                            const char* row_ptr = reinterpret_cast<const char*>(data.data()) + (i * row_stride);
                            of.write(row_ptr, row_stride);
                        }
                    }
                    else{
                        of.write((const char*)data.data(), data.size());
                    }
                }
                else {
                    uint32_t new_stride = Make_Stride_Aligned(4);
                    std::vector<uint8_t> padding_row(new_stride - row_stride);

                    of.write((const char*)&file_header, sizeof(file_header));
                    of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
                    if(bmp_info_header.bit_count == 32) {
                        of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
                    }


                    if(bmp_info_header.height > 0){
                        for (int32_t y = bmp_info_header.height - 1; y >= 0; --y) {
                            of.write(reinterpret_cast<const char*>(data.data() + row_stride * y), row_stride);
                            if (!padding_row.empty()) {
                                of.write(reinterpret_cast<const char*>(padding_row.data()), padding_row.size());
                            }
                        }
                    }
                    else{
                        for (int y = 0; y < -bmp_info_header.height; ++y) {
                            of.write((const char*)(data.data() + row_stride * y), row_stride);
                            of.write((const char*)padding_row.data(), padding_row.size());
                        }
                    }
                }
            }
            else {
                throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
            }
        }
        else {
            throw std::runtime_error("Unable to open the output image file.");
        }
        */
    }

    uint32_t BMP::Make_Stride_Aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }

    void BMP::Check_Color_Header(ColorHeader const& _bmp_color_header) {
        ColorHeader expected_color_header;
        if(expected_color_header.red_mask != _bmp_color_header.red_mask ||
            expected_color_header.blue_mask != _bmp_color_header.blue_mask ||
            expected_color_header.green_mask != _bmp_color_header.green_mask ||
            expected_color_header.alpha_mask != _bmp_color_header.alpha_mask) {
            throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
        }
        if(expected_color_header.color_space_type != _bmp_color_header.color_space_type) {
            throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
        }
    }
} //namespace EWE
