#pragma once

#include <cstdint>

namespace ImageProcessor{
    
    enum class NumericFormat : uint8_t {
        INVALID = 0,
        UNORM,  // unsigned normalized [0, 1]
        SNORM,  // normalized [-1, 1]
        UINT,
        SINT,   
        UFLOAT,
        FLOAT,
        SRGB,    // gamma corrected UNorm
        DEPTH
    };

    //in this enum, each compression type is given a comprehensible name
    //so that the developer has some kind of way to know what kind of operation is performed
    enum class ComprehensibleCompressionFormat : uint8_t{

        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6,
        BC7,
        BC8,

        NONE,
    };

    //in this struct, each distinct format is given an arbitrary value, via enum
    enum class Channel : uint8_t { R = 0, G = 1, B = 2, A = 3, ZERO = 4, ONE = 5 };
    struct UndecipherableImageFormat{
        union{ //these widths are in bits, not bytes
            struct{
                uint8_t red_width;
                uint8_t green_width;
                uint8_t blue_width;
                uint8_t alpha_width;
            };
            uint8_t depth_width;
        };
        NumericFormat format;
        ComprehensibleCompressionFormat compression;
        Channel swizzle[4]{Channel::R, Channel::G, Channel::B, Channel::A};
    };

#ifdef USING_VULKAN_CONVERSION //copy this over to file that includes vulkan.h

#ifndef VULKAN_CORE_H_
    #error "Vulkan needs to be included before this file is"
#endif

    constexpr VkFormat ConvertFormatToVulkan(UndecipherableImageFormat const& format){
        if (format.compression != ComprehensibleCompressionFormat::NONE) {
            switch (format.compression) {
                case ComprehensibleCompressionFormat::BC1:
                    if (format.format == NumericFormat::SRGB) 
                        return format.alpha_width > 0 ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK : VK_FORMAT_BC1_RGB_SRGB_BLOCK;
                    return format.alpha_width > 0 ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
                
                case ComprehensibleCompressionFormat::BC2:
                    return (format.format == NumericFormat::SRGB) ? VK_FORMAT_BC2_SRGB_BLOCK : VK_FORMAT_BC2_UNORM_BLOCK;
                
                case ComprehensibleCompressionFormat::BC3:
                    return (format.format == NumericFormat::SRGB) ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
                
                case ComprehensibleCompressionFormat::BC4:
                    return (format.format == NumericFormat::SNORM) ? VK_FORMAT_BC4_SNORM_BLOCK : VK_FORMAT_BC4_UNORM_BLOCK;
                
                case ComprehensibleCompressionFormat::BC5:
                    return (format.format == NumericFormat::SNORM) ? VK_FORMAT_BC5_SNORM_BLOCK : VK_FORMAT_BC5_UNORM_BLOCK;
                
                case ComprehensibleCompressionFormat::BC7:
                    return (format.format == NumericFormat::SRGB) ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
                
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        const uint32_t total_bits = format.red_width + format.green_width + format.blue_width + format.alpha_width;
        const bool is_bgra = (format.swizzle[0] == Channel::B && format.swizzle[2] == Channel::R);

        switch (total_bits) {
            case 8:
                if (format.alpha_width == 8) {
                    return VK_FORMAT_A8_UNORM_KHR;
                }
                switch(format.format){
                    case NumericFormat::UNORM: return VK_FORMAT_R8_UNORM;
                    case NumericFormat::SNORM: return VK_FORMAT_R8_SNORM;
                    case NumericFormat::UINT:  return VK_FORMAT_R8_UINT;
                    case NumericFormat::SINT:  return VK_FORMAT_R8_SINT;
                    default: return VK_FORMAT_UNDEFINED;
                }
                break;

            case 16:
                if (format.red_width == 8 && format.green_width == 8) {
                    switch(format.format) {
                        case NumericFormat::UNORM: return VK_FORMAT_R8G8_UNORM;
                        case NumericFormat::SNORM: return VK_FORMAT_R8G8_SNORM;
                        case NumericFormat::UINT:  return VK_FORMAT_R8G8_UINT;
                        case NumericFormat::SINT:  return VK_FORMAT_R8G8_SINT;
                        case NumericFormat::SRGB:  return VK_FORMAT_R8G8_SRGB;
                        default: return VK_FORMAT_UNDEFINED;
                    }
                }
                if (format.red_width == 16) {
                    switch(format.format) {
                        case NumericFormat::FLOAT: return VK_FORMAT_R16_SFLOAT;
                        case NumericFormat::UNORM: return VK_FORMAT_R16_UNORM;
                        case NumericFormat::SNORM: return VK_FORMAT_R16_SNORM;
                        case NumericFormat::UINT:  return VK_FORMAT_R16_UINT;
                        case NumericFormat::SINT:  return VK_FORMAT_R16_SINT;
                        case NumericFormat::DEPTH: return VK_FORMAT_D16_UNORM;
                        default: return VK_FORMAT_UNDEFINED;
                    }
                }
                if (format.red_width == 5 && format.green_width == 6) {
                    return is_bgra ? VK_FORMAT_B5G6R5_UNORM_PACK16 : VK_FORMAT_R5G6B5_UNORM_PACK16;
                }
                if (format.red_width == 5 && format.alpha_width == 1) {
                    return is_bgra ? VK_FORMAT_B5G5R5A1_UNORM_PACK16 : VK_FORMAT_A1R5G5B5_UNORM_PACK16;
                }
                if (format.red_width == 4) {
                    return is_bgra ? VK_FORMAT_B4G4R4A4_UNORM_PACK16 : VK_FORMAT_R4G4B4A4_UNORM_PACK16;
                }
                break;
            case 24:
                if (format.red_width == 8) {
                    if (format.format == NumericFormat::SRGB) {
                        return is_bgra ? VK_FORMAT_B8G8R8_SRGB : VK_FORMAT_R8G8B8_SRGB;
                    }
                    else{
                        return is_bgra ? VK_FORMAT_B8G8R8_UNORM : VK_FORMAT_R8G8B8_UNORM;
                    }
                }
                break;
                
            case 32:
                if (format.red_width == 8 && format.green_width == 8 && format.blue_width == 8) {
                    if (format.format == NumericFormat::SRGB) {
                        return is_bgra ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;
                    }
                    switch (format.format) {
                        case NumericFormat::UINT: return is_bgra ? VK_FORMAT_B8G8R8A8_UINT : VK_FORMAT_R8G8B8A8_UINT;
                        case NumericFormat::SINT: return is_bgra ? VK_FORMAT_B8G8R8A8_SINT : VK_FORMAT_R8G8B8A8_SINT;
                        case NumericFormat::SNORM: return is_bgra ? VK_FORMAT_B8G8R8A8_SNORM : VK_FORMAT_R8G8B8A8_SNORM;
                        case NumericFormat::UNORM: return is_bgra ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
                        default: return VK_FORMAT_UNDEFINED;
                    }
                }
                if (format.red_width == 10) {
                    return (format.format == NumericFormat::UINT) ? 
                        VK_FORMAT_A2B10G10R10_UINT_PACK32 : VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                }
                break;

            case 64:
                if (format.red_width == 16) return VK_FORMAT_R16G16B16A16_SFLOAT;
                if (format.red_width == 32) return VK_FORMAT_R32G32_SFLOAT;
                break;

            case 96:
                return VK_FORMAT_R32G32B32_SFLOAT;

            case 128:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
        }

        return VK_FORMAT_UNDEFINED;
    }


    constexpr UndecipherableImageFormat ConvertVulkanToFormat(VkFormat vk_format) {
        UndecipherableImageFormat format{};

        //defaults
        format.compression = ComprehensibleCompressionFormat::NONE;
        format.format = NumericFormat::UNORM;
        format.swizzle[0] = Channel::R;
        format.swizzle[1] = Channel::G;
        format.swizzle[2] = Channel::B;
        format.swizzle[3] = Channel::A;

        switch (vk_format) {
        //8 bits
            case VK_FORMAT_R8_UNORM: format.red_width = 8; break;
            case VK_FORMAT_R8_SRGB:  format.red_width = 8; format.format = NumericFormat::SRGB; break;
            case VK_FORMAT_R8_UINT:  format.red_width = 8; format.format = NumericFormat::UINT; break;

        //16 bits
            case VK_FORMAT_R8G8_UNORM: format.red_width = 8; format.green_width = 8; break;
            case VK_FORMAT_R16_SFLOAT: format.red_width = 16; format.format = NumericFormat::FLOAT; break;
            case VK_FORMAT_R16_UNORM:  format.red_width = 16; break;
            
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
                format.red_width = 5; format.green_width = 6; format.blue_width = 5;
                format.swizzle[0] = Channel::B; format.swizzle[2] = Channel::R;
                break;

        //24 bits
            case VK_FORMAT_R8G8B8_UNORM: 
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; 
                break;
            case VK_FORMAT_B8G8R8_UNORM:
                format.red_width = 8; format.green_width = 8; format.blue_width = 8;
                format.swizzle[0] = Channel::B; format.swizzle[2] = Channel::R;
                break;

        //32 bits
            case VK_FORMAT_R8G8B8A8_UNORM:
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; format.alpha_width = 8;
                break;
            case VK_FORMAT_R8G8B8A8_SRGB:
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; format.alpha_width = 8;
                format.format = NumericFormat::SRGB;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; format.alpha_width = 8;
                format.swizzle[0] = Channel::B; format.swizzle[2] = Channel::R;
                break;
            case VK_FORMAT_B8G8R8A8_SRGB:
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; format.alpha_width = 8;
                format.format = NumericFormat::SRGB;
                format.swizzle[0] = Channel::B; format.swizzle[2] = Channel::R;
                break;

        //depth
            case VK_FORMAT_D16_UNORM: format.depth_width = 16; format.format = NumericFormat::DEPTH; break;
            case VK_FORMAT_D32_SFLOAT: format.depth_width = 32; format.format = NumericFormat::DEPTH; break;

        //compressed
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
                format.compression = ComprehensibleCompressionFormat::BC1;
                format.red_width = 5; format.green_width = 6; format.blue_width = 5;
                break;
            case VK_FORMAT_BC3_UNORM_BLOCK:
                format.compression = ComprehensibleCompressionFormat::BC3;
                format.red_width = 8; format.green_width = 8; format.blue_width = 8; format.alpha_width = 8;
                break;
        //invalid
            default:
                format.format = NumericFormat::INVALID;
                break;
        }

        return format;
    }
#endif
}