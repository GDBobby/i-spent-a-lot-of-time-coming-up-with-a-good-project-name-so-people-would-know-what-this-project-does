#include "ImageProcessor.h"
#include "BMP.h"

//#include <utils/io.hpp>

#include <cstring> //for memcmp
#include <cmath>

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmicrosoft-enum-value"
    #pragma clang diagnostic ignored "-Wunused-but-set-variable"
    #pragma clang diagnostic ignored "-Wconversion"
    #pragma clang diagnostic ignored "-Wsign-conversion"
    #pragma clang diagnostic ignored "-Wold-style-cast"
    #pragma clang diagnostic ignored "-Wdouble-promotion"
    #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
    #pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#include <dds.hpp>

#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__JPEG
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__ZLIB

#define WUFFS_CONFIG__STATIC_FUNCTIONS

#ifndef WUFFS_IMPLEMENTATION
#define WUFFS_IMPLEMENTATION
#endif

#include <wuffs-v0.4.c>

//#include <bc7enc.h>
//#include <rgbcx.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#define s_cast static_cast
#define r_cast reinterpret_cast

namespace ImageProcessor {
    enum class ImageFileType{
        DDS,
        JPG,
        PNG,
        BMP,
        INVALID
    };

/*
    constexpr ImageFileType GetFileTypeFromExtension(std::filesystem::path const& extension){
        if(extension == ".jpg" || extension == ".jpeg"){
            return ImageFileType::JPG;
        }
        else if(extension == ".png"){
            return ImageFileType::PNG;
        }
        else if(extension == ".dds"){
            return ImageFileType::DDS;
        }
        return ImageFileType::INVALID;;
    }
    constexpr ImageFileType GetFileTypeFromPath(std::filesystem::path const& path){
        return GetFileTypeFromPath(path.extension());
    }
*/

    static ImageFileType detect_image_type(std::span<std::byte const> data) {
        if(data.size() < 8) {
            return ImageFileType::INVALID;
        }

        // PNG magic bytes: 89 50 4E 47 0D 0A 1A 0A
        constexpr std::array<std::byte, 8> png_magic = {
            std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
            std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
        };
        if(std::memcmp(data.data(), png_magic.data(), 8) == 0) {
            return ImageFileType::PNG;
        }

        // JPEG magic bytes: FF D8 FF (SOI marker + APP0/APP1/etc)
        if(
           data[0] == std::byte{0xFF} &&
           data[1] == std::byte{0xD8} &&
           data[2] == std::byte{0xFF}) {
            return ImageFileType::JPG;
        }

        // DDS magic bytes: "DDS " (44 44 53 20)
        if(
           data[0] == std::byte{0x44} &&
           data[1] == std::byte{0x44} &&
           data[2] == std::byte{0x53} &&
           data[3] == std::byte{0x20}) {
            return ImageFileType::DDS;
        }

        if(data[1] == std::byte{0x4D} && data[0] == std::byte{0x42}){
            return ImageFileType::BMP;
        }

        return ImageFileType::INVALID;
    }

    static UndecipherableImageFormat Deciphering_DDS_Format(DXGI_FORMAT format, bool alpha_flag) {
        switch(format) {
            // --- Block Compressed Formats ---
            case DXGI_FORMAT_BC1_UNORM:
                return alpha_flag 
                    ? UndecipherableImageFormat{{4,4,4,4}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC1}
                    : UndecipherableImageFormat{{5,6,5,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC1};
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return alpha_flag 
                    ? UndecipherableImageFormat{{4,4,4,4}, NumericFormat::SRGB, ComprehensibleCompressionFormat::BC1}
                    : UndecipherableImageFormat{{5,6,5,0}, NumericFormat::SRGB, ComprehensibleCompressionFormat::BC1};

            case DXGI_FORMAT_BC2_UNORM:      return {{8,8,8,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC2};
            case DXGI_FORMAT_BC2_UNORM_SRGB: return {{8,8,8,8}, NumericFormat::SRGB,  ComprehensibleCompressionFormat::BC2};
            case DXGI_FORMAT_BC3_UNORM:      return {{8,8,8,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC3};
            case DXGI_FORMAT_BC3_UNORM_SRGB: return {{8,8,8,8}, NumericFormat::SRGB,  ComprehensibleCompressionFormat::BC3};
            
            case DXGI_FORMAT_BC4_UNORM:      return {{8,0,0,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC4};
            case DXGI_FORMAT_BC4_SNORM:      return {{8,0,0,0}, NumericFormat::SNORM, ComprehensibleCompressionFormat::BC4};
            case DXGI_FORMAT_BC5_UNORM:      return {{8,8,0,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC5};
            case DXGI_FORMAT_BC5_SNORM:      return {{8,8,0,0}, NumericFormat::SNORM, ComprehensibleCompressionFormat::BC5};
            
            case DXGI_FORMAT_BC7_UNORM:      return {{8,8,8,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::BC7};
            case DXGI_FORMAT_BC7_UNORM_SRGB: return {{8,8,8,8}, NumericFormat::SRGB,  ComprehensibleCompressionFormat::BC7};

            // --- 8-bit wide ---
            case DXGI_FORMAT_R8_UNORM:  return {{8,0,0,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R8_UINT:   return {{8,0,0,0}, NumericFormat::UINT,  ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R8_SNORM:  return {{8,0,0,0}, NumericFormat::SNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R8_SINT:   return {{8,0,0,0}, NumericFormat::SINT,  ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_A8_UNORM:  return {{0,0,0,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};

            // --- 16-bit wide ---
            case DXGI_FORMAT_R8G8_UNORM: return {{8,8,0,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R8G8_UINT:  return {{8,8,0,0}, NumericFormat::UINT,  ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R16_FLOAT:  return {{16,0,0,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R16_UNORM:  return {{16,0,0,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            
            case DXGI_FORMAT_B5G6R5_UNORM:   return {{5,6,5,0}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_B5G5R5A1_UNORM: return {{5,5,5,1}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_B4G4R4A4_UNORM: return {{4,4,4,4}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};

            // --- 32-bit wide ---
            case DXGI_FORMAT_R8G8B8A8_UNORM:      return {{8,8,8,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return {{8,8,8,8}, NumericFormat::SRGB,  ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_B8G8R8A8_UNORM:      return {{8,8,8,8}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R16G16_FLOAT:        return {{16,16,0,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R32_FLOAT:           return {{32,0,0,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};
            
            case DXGI_FORMAT_R10G10B10A2_UNORM:   return {{10,10,10,2}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R11G11B10_FLOAT:     return {{11,11,10,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:  return {{9,9,9,5}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};

            // --- 64-bit wide ---
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return {{16,16,16,16}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R16G16B16A16_UNORM: return {{16,16,16,16}, NumericFormat::UNORM, ComprehensibleCompressionFormat::NONE};
            case DXGI_FORMAT_R32G32_FLOAT:       return {{32,32,0,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};

            // --- 96-bit wide ---
            case DXGI_FORMAT_R32G32B32_FLOAT: return {{32,32,32,0}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};

            // --- 128-bit wide ---
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return {{32,32,32,32}, NumericFormat::FLOAT, ComprehensibleCompressionFormat::NONE};

            default:
                return {{0,0,0,0}, NumericFormat::INVALID, ComprehensibleCompressionFormat::NONE};
        }
    }

    bool RawImage::ProcessData(std::span<std::byte> preprocessed_data) {
        ImageFileType image_type = detect_image_type(preprocessed_data);

        switch(image_type) {
            case ImageFileType::PNG: return LoadPNG(preprocessed_data);
            case ImageFileType::JPG: return LoadJPG(preprocessed_data);
            case ImageFileType::DDS: return LoadDDS(preprocessed_data);
            case ImageFileType::BMP: return LoadBMP(preprocessed_data);
            case ImageFileType::INVALID:
            default: return false;
        }
    }

    bool RawImage::LoadDDS(std::span<std::byte> preprocessed_data) noexcept {
        dds::Image dds_image = {};
        dds::ReadResult result = dds::readImage(const_cast<uint8_t*>(r_cast<const uint8_t*>(preprocessed_data.data())), preprocessed_data.size(), &dds_image);
        if(result != dds::ReadResult::Success) {
            return false;
        }

        format = Deciphering_DDS_Format(dds_image.format, dds_image.supportsAlpha);
        width = dds_image.width;
        height = dds_image.height;
        depth = dds_image.depth;

        std::size_t total_data_size = 0;
        for(const auto& mipmap : dds_image.mipmaps) {
            total_data_size += mipmap.size();
        }
        processed_data.resize(total_data_size);

        std::size_t current_offset = 0;
        mipmaps.reserve(dds_image.mipmaps.size());
        for(const auto& mipmap : dds_image.mipmaps) {
            std::memcpy(processed_data.data() + current_offset, mipmap.data(), mipmap.size());
            mipmaps.emplace_back(processed_data.data() + current_offset, mipmap.size());
            current_offset += mipmap.size();
        }

        return true;
    }

#define WORKBUF_STACK_SIZE 8192

    bool RawImage::LoadPNG(std::span<std::byte> preprocessed_data) noexcept {

        wuffs_png__decoder dec;
        wuffs_base__status status = wuffs_png__decoder__initialize(&dec, sizeof(wuffs_png__decoder), WUFFS_VERSION, 0);
        if(!wuffs_base__status__is_ok(&status)) {
            return false;
        }

        wuffs_png__decoder__set_quirk(&dec, WUFFS_BASE__QUIRK_IGNORE_CHECKSUM, 1);

        wuffs_base__io_buffer io_buf = wuffs_base__ptr_u8__reader(const_cast<uint8_t*>(r_cast<const uint8_t*>(preprocessed_data.data())), preprocessed_data.size(), true);
        wuffs_base__image_config config = {};

        status = wuffs_png__decoder__decode_image_config(&dec, &config, &io_buf);
        if(!wuffs_base__status__is_ok(&status)) {
            return false;
        }

        width = wuffs_base__pixel_config__width(&config.pixcfg);
        height = wuffs_base__pixel_config__height(&config.pixcfg);

        //loaded_image.format = daxa::Format::R8G8B8A8_UNORM;
        format = UndecipherableImageFormat{
            .red_width = 8,
            .green_width = 8,
            .blue_width = 8,
            .alpha_width = 8,
            .format = NumericFormat::UNORM,
            .compression = ComprehensibleCompressionFormat::NONE
        };

        wuffs_base__pixel_format src_pfmt = wuffs_base__pixel_config__pixel_format(&config.pixcfg);
        uint32_t coloration = wuffs_base__pixel_format__coloration(&src_pfmt);
        uint32_t transparency = wuffs_base__pixel_format__transparency(&src_pfmt);

        int src_channels = 0;
        if(coloration == WUFFS_BASE__PIXEL_COLORATION__GRAY) {
            src_channels = (transparency == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE) ? 1 : 2;
        } else { // RICH
            src_channels = (transparency == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE) ? 3 : 4;
        }

        int desired_channels = 4;
        // if (channels_in_file) *channels_in_file = src_channels;

        int n = (desired_channels != 0) ? desired_channels : src_channels;
        uint32_t dst_pfmt = 0;
        bool is_16bit = false;

        if(is_16bit) {
            dst_pfmt = (n == 1) ? WUFFS_BASE__PIXEL_FORMAT__Y_16LE : WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL_4X16LE;
            if(n == 3)
                n = 4; // Force 4 channels for RGB -> RGBA conversion in 16-bit mode
        } else {
            switch(n) {
                case 1: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__Y; break;
                case 2: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__YA_NONPREMUL; break;
                case 3: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__RGB; break;
                default: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL; break;
            }
        }

        wuffs_base__pixel_config__set(&config.pixcfg, dst_pfmt, WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);
        std::size_t bytes_per_pixel = s_cast<std::size_t>(is_16bit ? (n + n) : n);

        if(width == 0 || height == 0 || bytes_per_pixel == 0 ||
           s_cast<std::size_t>(width) > SIZE_MAX / height ||
           s_cast<std::size_t>(width * height) > SIZE_MAX / bytes_per_pixel
        ) {
            return false;
        }

        std::size_t pixel_buf_size = s_cast<std::size_t>(width * height) * bytes_per_pixel;
        processed_data.resize(pixel_buf_size);

        wuffs_base__range_ii_u64 workbuf_len_range = wuffs_png__decoder__workbuf_len(&dec);
        void* workbuf_ptr = nullptr;

        wuffs_base__slice_u8 workbuf = {};

        if(workbuf_len_range.max_incl > 0) {
            std::size_t wb_len = s_cast<std::size_t>(workbuf_len_range.max_incl);
            if(wb_len > SIZE_MAX) {
                return false;
            }

            uint8_t workbuf_stack[WORKBUF_STACK_SIZE];
            if(wb_len <= WORKBUF_STACK_SIZE) {
                workbuf.ptr = workbuf_stack;
                workbuf.len = wb_len;
            } else {
                workbuf_ptr = malloc(wb_len);
                if(!workbuf_ptr) {
                    return false;
                }
                workbuf.ptr = s_cast<uint8_t*>(workbuf_ptr);
                workbuf.len = wb_len;
            }
        }

        wuffs_base__pixel_buffer pb = {};
        status = wuffs_base__pixel_buffer__set_from_slice(&pb, &config.pixcfg, wuffs_base__make_slice_u8(r_cast<uint8_t*>(processed_data.data()), pixel_buf_size));
        if(!wuffs_base__status__is_ok(&status)) {
            if(workbuf_ptr != nullptr) {
                free(workbuf_ptr);
            }
            return false;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
        status = wuffs_png__decoder__decode_frame(&dec, &pb, &io_buf, WUFFS_BASE__PIXEL_BLEND__SRC, workbuf, NULL);
#pragma clang diagnostic pop

        if(workbuf_ptr != nullptr) {
            free(workbuf_ptr);
        }

        return wuffs_base__status__is_ok(&status);
    }

    bool RawImage::LoadBMP(std::span<std::byte> preprocessed_data) noexcept {
        //move bmp reading to wuffs eventually
        //wuffs_bmp__decoder__initialize();
        BMP bmp{};
        std::size_t current_offset = 0;
        BMP::FileHeader fileHeader;
        memcpy(&fileHeader, &preprocessed_data[current_offset], sizeof(BMP::FileHeader));
        BMP::InfoHeader infoHeader;
        current_offset += sizeof(BMP::FileHeader);
        memcpy(&infoHeader, &preprocessed_data[current_offset], sizeof(BMP::InfoHeader));
        current_offset += sizeof(BMP::InfoHeader);


        if(infoHeader.compression != 0 && infoHeader.compression != 3){ //idk what 3 is, but for now its acceptable
            return false;
        }
        if(infoHeader.bit_count != 24 && infoHeader.bit_count != 32){
            return false; //not ready to support others yet yet
        }

        BMP::ColorHeader colorHeader{};
        const bool using_color_header = infoHeader.size >= (sizeof(BMP::InfoHeader) + sizeof(BMP::ColorHeader));
        if(using_color_header){
            memcpy(&colorHeader, &preprocessed_data[current_offset], sizeof(BMP::ColorHeader));
        }

        //now jump to pixel beginning
        current_offset = fileHeader.offset_data;

        format.compression = ComprehensibleCompressionFormat::NONE;
        format.format = NumericFormat::UNORM;
        format.red_width = 8;
        format.green_width = 8;
        format.blue_width = 8;
        format.swizzle[0] = Channel::B;
        format.swizzle[1] = Channel::G;
        format.swizzle[2] = Channel::R;
        if(infoHeader.bit_count == 32){
            format.alpha_width = 8;
        }
        else if(infoHeader.bit_count == 24){
            format.alpha_width = 0;
        }

        
        width = infoHeader.width;
        //this height is the RawImage height
        height = std::abs(infoHeader.height);
        const std::size_t raw_data_size = infoHeader.width * height * infoHeader.bit_count / 8;//0x7fff uses a bit mask to remove the sign

        processed_data.resize(raw_data_size);
        memcpy(processed_data.data(), (preprocessed_data.begin() + current_offset).base(), raw_data_size);

        if(infoHeader.height > 0){
            //bmp is stored bottom to top, goign to flip it
            const std::size_t row_stride = infoHeader.width * (infoHeader.bit_count / 8);
            for (std::size_t i = 0; i < height / 2; i++) {
                std::swap_ranges(
                    processed_data.begin() + (i * row_stride),
                    processed_data.begin() + ((i + 1) * row_stride),
                    processed_data.begin() + (height - 1 - i) * row_stride
                );
            }
        }
        //this is height flipped, can still be handled

        return true;
    }
    bool RawImage::LoadJPG(std::span<std::byte> preprocessed_data) {

        wuffs_jpeg__decoder dec;
        wuffs_base__status status = wuffs_jpeg__decoder__initialize(&dec, sizeof(wuffs_jpeg__decoder), WUFFS_VERSION, 0);
        if(!wuffs_base__status__is_ok(&status)) {
            throw std::runtime_error(status.repr);
            return false;
        }

        wuffs_jpeg__decoder__set_quirk(&dec, WUFFS_BASE__QUIRK_IGNORE_CHECKSUM, 1);

        wuffs_base__io_buffer io_buf = wuffs_base__ptr_u8__reader(const_cast<uint8_t*>(r_cast<const uint8_t*>(preprocessed_data.data())), preprocessed_data.size(), true);
        wuffs_base__image_config config = {};

        status = wuffs_jpeg__decoder__decode_image_config(&dec, &config, &io_buf);
        if(!wuffs_base__status__is_ok(&status)) {
            throw std::runtime_error(status.repr);
            return false;
        }

        width = wuffs_base__pixel_config__width(&config.pixcfg);
        height = wuffs_base__pixel_config__height(&config.pixcfg);

        format = UndecipherableImageFormat{
            .red_width = 8,
            .green_width = 8,
            .blue_width = 8,
            .alpha_width = 8,
            .format = NumericFormat::UNORM,
            .compression = ComprehensibleCompressionFormat::NONE
        };

        wuffs_base__pixel_format src_pfmt = wuffs_base__pixel_config__pixel_format(&config.pixcfg);
        uint32_t coloration = wuffs_base__pixel_format__coloration(&src_pfmt);
        uint32_t transparency = wuffs_base__pixel_format__transparency(&src_pfmt);

        int src_channels = 0;
        if(coloration == WUFFS_BASE__PIXEL_COLORATION__GRAY) {
            src_channels = (transparency == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE) ? 1 : 2;
        } else { // RICH
            src_channels = (transparency == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE) ? 3 : 4;
        }

        int desired_channels = 4;
        // if (channels_in_file) *channels_in_file = src_channels;

        int n = (desired_channels != 0) ? desired_channels : src_channels;
        uint32_t dst_pfmt = 0;

        switch(n) {
            case 1: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__Y; break;
            case 2: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__YA_NONPREMUL; break;
            case 3: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__RGB; break;
            default: dst_pfmt = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL; break;
        }

        wuffs_base__pixel_config__set(&config.pixcfg, dst_pfmt, WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);

        std::size_t bytes_per_pixel = s_cast<std::size_t>(n);

        if(width == 0 || height == 0 || bytes_per_pixel == 0 ||
           s_cast<std::size_t>(width) > SIZE_MAX / height ||
           s_cast<std::size_t>(width * height) > SIZE_MAX / bytes_per_pixel) {
            return false;
        }

        std::size_t pixel_buf_size = s_cast<std::size_t>(width * height) * bytes_per_pixel;
        processed_data.resize(pixel_buf_size);

        wuffs_base__range_ii_u64 workbuf_len_range = wuffs_jpeg__decoder__workbuf_len(&dec);
        void* workbuf_ptr = nullptr;

        wuffs_base__slice_u8 workbuf = {};

        if(workbuf_len_range.max_incl > 0) {
            std::size_t wb_len = s_cast<std::size_t>(workbuf_len_range.max_incl);
            if(wb_len > SIZE_MAX) {
                return false;
            }

            uint8_t workbuf_stack[WORKBUF_STACK_SIZE];
            if(wb_len <= WORKBUF_STACK_SIZE) {
                workbuf.ptr = workbuf_stack;
                workbuf.len = wb_len;
            } else {
                workbuf_ptr = malloc(wb_len);
                if(workbuf_ptr == nullptr) {
                    return false;
                }
                workbuf.ptr = s_cast<uint8_t*>(workbuf_ptr);
                workbuf.len = wb_len;
            }
        }

        wuffs_base__pixel_buffer pb = {};
        status = wuffs_base__pixel_buffer__set_from_slice(&pb, &config.pixcfg, wuffs_base__make_slice_u8(r_cast<uint8_t*>(processed_data.data()), pixel_buf_size));
        if(!wuffs_base__status__is_ok(&status)) {
            if(workbuf_ptr != nullptr) {
                free(workbuf_ptr);
            }
            throw std::runtime_error(status.repr);
            return false;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
        status = wuffs_jpeg__decoder__decode_frame(&dec, &pb, &io_buf, WUFFS_BASE__PIXEL_BLEND__SRC, workbuf, NULL);
#pragma clang diagnostic pop

        if(workbuf_ptr != nullptr) {
            free(workbuf_ptr);
        }
        if(!wuffs_base__status__is_ok(&status)) {
            throw std::runtime_error(status.repr);
            return false;
        }
        return true;
    }

    bool has_alpha_channel(const RawImage& image) {

        for(std::size_t i = 3; i < image.processed_data.size(); i += 4) {
            if(s_cast<uint8_t>(image.processed_data[i]) < 255) {
                return true;
            }
        }

        return false;
    }

    void downsample_rgba8(const std::byte *src, std::byte *dst, uint32_t src_width, uint32_t src_height) {

        uint32_t dst_width = std::max(1u, src_width / 2);
        uint32_t dst_height = std::max(1u, src_height / 2);

        for(uint32_t y = 0; y < dst_height; y++) {
            for(uint32_t x = 0; x < dst_width; x++) {
                uint32_t src_x = x * 2;
                uint32_t src_y = y * 2;

                uint32_t r = 0, g = 0, b = 0, a = 0;
                for(uint32_t dy = 0; dy < 2 && (src_y + dy) < src_height; dy++) {
                    for(uint32_t dx = 0; dx < 2 && (src_x + dx) < src_width; dx++) {
                        uint32_t src_idx = ((src_y + dy) * src_width + (src_x + dx)) * 4;
                        r += s_cast<uint32_t>(src[src_idx + 0]);
                        g += s_cast<uint32_t>(src[src_idx + 1]);
                        b += s_cast<uint32_t>(src[src_idx + 2]);
                        a += s_cast<uint32_t>(src[src_idx + 3]);
                    }
                }

                uint32_t dst_idx = (y * dst_width + x) * 4;
                dst[dst_idx + 0] = s_cast<std::byte>(r / 4);
                dst[dst_idx + 1] = s_cast<std::byte>(g / 4);
                dst[dst_idx + 2] = s_cast<std::byte>(b / 4);
                dst[dst_idx + 3] = s_cast<std::byte>(a / 4);
            }
        }
    }

    void downsample_rgba8_premultiplied(const std::byte* src, std::byte* dst, uint32_t src_width, uint32_t src_height, bool is_srgb) {

        uint32_t dst_width = std::max(1u, src_width / 2);
        uint32_t dst_height = std::max(1u, src_height / 2);

        auto srgb_to_linear = [](uint8_t v) -> float {
            float f = s_cast<float>(v) / 255.0f;
            return (f <= 0.04045f) ? f / 12.92f : std::pow((f + 0.055f) / 1.055f, 2.4f);
        };

        auto linear_to_srgb = [](float v) -> uint8_t {
            float s = (v <= 0.0031308f) ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
            return s_cast<uint8_t>(std::clamp(s * 255.0f + 0.5f, 0.0f, 255.0f));
        };

        for(uint32_t y = 0; y < dst_height; y++) {
            for(uint32_t x = 0; x < dst_width; x++) {
                uint32_t src_x = x * 2;
                uint32_t src_y = y * 2;

                float pr = 0.0f, pg = 0.0f, pb = 0.0f, pa = 0.0f;
                uint32_t sample_count = 0;

                for(uint32_t dy = 0; dy < 2 && (src_y + dy) < src_height; dy++) {
                    for(uint32_t dx = 0; dx < 2 && (src_x + dx) < src_width; dx++) {
                        uint32_t src_idx = ((src_y + dy) * src_width + (src_x + dx)) * 4;
                        float a = s_cast<float>(s_cast<uint8_t>(src[src_idx + 3])) / 255.0f;

                        float r, g, b;
                        if(is_srgb) {
                            r = srgb_to_linear(s_cast<uint8_t>(src[src_idx + 0]));
                            g = srgb_to_linear(s_cast<uint8_t>(src[src_idx + 1]));
                            b = srgb_to_linear(s_cast<uint8_t>(src[src_idx + 2]));
                        } else {
                            r = s_cast<float>(s_cast<uint8_t>(src[src_idx + 0])) / 255.0f;
                            g = s_cast<float>(s_cast<uint8_t>(src[src_idx + 1])) / 255.0f;
                            b = s_cast<float>(s_cast<uint8_t>(src[src_idx + 2])) / 255.0f;
                        }

                        pr += r * a;
                        pg += g * a;
                        pb += b * a;
                        pa += a;
                        sample_count++;
                    }
                }

                float inv_count = 1.0f / s_cast<float>(sample_count);
                pr *= inv_count;
                pg *= inv_count;
                pb *= inv_count;
                pa *= inv_count;

                float r, g, b;
                if(pa > 1e-6f) {
                    r = pr / pa;
                    g = pg / pa;
                    b = pb / pa;
                } else {
                    r = g = b = 0.0f;
                }

                uint32_t dst_idx = (y * dst_width + x) * 4;
                if(is_srgb) {
                    dst[dst_idx + 0] = s_cast<std::byte>(linear_to_srgb(r));
                    dst[dst_idx + 1] = s_cast<std::byte>(linear_to_srgb(g));
                    dst[dst_idx + 2] = s_cast<std::byte>(linear_to_srgb(b));
                } else {
                    dst[dst_idx + 0] = s_cast<std::byte>(s_cast<uint8_t>(std::clamp(r * 255.0f + 0.5f, 0.0f, 255.0f)));
                    dst[dst_idx + 1] = s_cast<std::byte>(s_cast<uint8_t>(std::clamp(g * 255.0f + 0.5f, 0.0f, 255.0f)));
                    dst[dst_idx + 2] = s_cast<std::byte>(s_cast<uint8_t>(std::clamp(b * 255.0f + 0.5f, 0.0f, 255.0f)));
                }
                dst[dst_idx + 3] = s_cast<std::byte>(s_cast<uint8_t>(std::clamp(pa * 255.0f + 0.5f, 0.0f, 255.0f)));
            }
        }
    }

    float compute_alpha_coverage(const std::byte* data, uint32_t pixel_count, float alpha_ref) {
        uint32_t count = 0;
        uint8_t ref_byte = s_cast<uint8_t>(alpha_ref * 255.0f);
        for(uint32_t i = 0; i < pixel_count; ++i) {
            if(s_cast<uint8_t>(data[i * 4 + 3]) > ref_byte) {
                ++count;
            }
        }
        return s_cast<float>(count) / s_cast<float>(pixel_count);
    }

    void scale_alpha_to_coverage(std::byte* data, uint32_t pixel_count, float desired_coverage, float alpha_ref) {
        if(desired_coverage <= 0.0f || desired_coverage >= 1.0f) { return; }
        if(pixel_count == 0) { return; }

        float lo = 0.0f, hi = 1.0f;
        for(uint32_t iter = 0; iter < 16; ++iter) {
            float mid = (lo + hi) * 0.5f;
            float coverage = compute_alpha_coverage(data, pixel_count, mid);
            if(coverage < desired_coverage) {
                hi = mid;
            } else {
                lo = mid;
            }
        }

        float a_r = (lo + hi) * 0.5f;
        if(a_r < 1e-6f) { return; }

        float scale = alpha_ref / a_r;

        for(uint32_t i = 0; i < pixel_count; ++i) {
            float a = s_cast<float>(s_cast<uint8_t>(data[i * 4 + 3])) / 255.0f;
            a = std::clamp(a * scale, 0.0f, 1.0f);
            data[i * 4 + 3] = s_cast<std::byte>(s_cast<uint8_t>(a * 255.0f + 0.5f));
        }
    }


    /*
    void RawImage::Generate_Mipmaps(bool preserve_alpha_coverage, float alpha_ref) {

        uint32_t max_dim = std::max(width, height);
        uint32_t mip_level_count = s_cast<uint32_t>(std::floor(std::log2(max_dim))) + 1;

        const bool is_srgb = format.format == NumericFormat::SRGB;

        float base_coverage = 0.0f;
        if(preserve_alpha_coverage) {
            base_coverage = compute_alpha_coverage(image.data.data(), image.width * image.height, alpha_ref);
        }

        std::vector<std::byte> prev_level = image.data;
        RawImage result{image.data};
        result.format = image.format;
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;

        result.mipmaps.emplace_back(image.data.data(), image.data.size());

        uint32_t current_width = image.width;
        uint32_t current_height = image.height;

        for(uint32_t mip = 1; mip < mip_level_count; ++mip) {
            uint32_t next_width = std::max(1u, current_width / 2);
            uint32_t next_height = std::max(1u, current_height / 2);

            std::vector<std::byte> next_level(next_width * next_height * 4);
            if(preserve_alpha_coverage) {
                downsample_rgba8_premultiplied(prev_level.data(), next_level.data(), current_width, current_height, is_srgb);
                scale_alpha_to_coverage(next_level.data(), next_width * next_height, base_coverage, alpha_ref);
            } else {
                downsample_rgba8(prev_level.data(), next_level.data(), current_width, current_height);
            }

            std::size_t prev_size = result.data.size();
            result.data.resize(prev_size + next_level.size());
            std::memcpy(result.data.data() + prev_size, next_level.data(), next_level.size());
            result.mipmaps.emplace_back(result.data.data() + prev_size, next_level.size());

            prev_level = std::move(next_level);
            current_width = next_width;
            current_height = next_height;
        }

        if(result.data.empty()) {
            result.data = image.data;
        } else {
            std::vector<std::byte> full_data;
            full_data.reserve(image.data.size() + result.data.size());
            full_data.insert(full_data.end(), image.data.begin(), image.data.end());
            full_data.insert(full_data.end(), result.data.begin(), result.data.end());
            result.data = std::move(full_data);

            result.mipmaps.clear();
            std::size_t offset = 0;
            current_width = image.width;
            current_height = image.height;
            for(uint32_t mip = 0; mip < mip_level_count; ++mip) {
                std::size_t mip_size = current_width * current_height * 4;
                result.mipmaps.emplace_back(result.data.data() + offset, mip_size);
                offset += mip_size;
                current_width = std::max(1u, current_width / 2);
                current_height = std::max(1u, current_height / 2);
            }
        }

        return result;
    }


    void initialize_compressor() {
        static bool compressor_initialized = false;
        if(!compressor_initialized) {
            bc7enc_compress_block_init();
            rgbcx::init();
            compressor_initialized = true;
        }
    }

    auto compress_to_bc1(RawLoadedImage& image, bool is_srgb) -> RawLoadedImage {
        

        initialize_compressor();

        RawLoadedImage result = {};
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;
        result.format = is_srgb ? daxa::Format::BC1_RGBA_SRGB_BLOCK : daxa::Format::BC1_RGBA_UNORM_BLOCK;

        uint32_t blocks_x = (image.width + 3) / 4;
        uint32_t blocks_y = (image.height + 3) / 4;
        std::size_t block_count = blocks_x * blocks_y;
        std::size_t compressed_size = block_count * 8;

        result.data.resize(compressed_size);
        result.mipmaps.emplace_back(result.data.data(), compressed_size);

        const std::byte *src = image.data.data();
        std::byte *dst = result.data.data();

        for(uint32_t by = 0; by < blocks_y; by++) {
            for(uint32_t bx = 0; bx < blocks_x; bx++) {
                uint8_t block_pixels[16 * 4];

                for(uint32_t y = 0; y < 4; y++) {
                    for(uint32_t x = 0; x < 4; x++) {
                        uint32_t src_x = std::min(bx * 4 + x, image.width - 1);
                        uint32_t src_y = std::min(by * 4 + y, image.height - 1);
                        uint32_t src_idx = (src_y * image.width + src_x) * 4;
                        uint32_t dst_idx = (y * 4 + x) * 4;

                        block_pixels[dst_idx + 0] = s_cast<uint8_t>(src[src_idx + 0]);
                        block_pixels[dst_idx + 1] = s_cast<uint8_t>(src[src_idx + 1]);
                        block_pixels[dst_idx + 2] = s_cast<uint8_t>(src[src_idx + 2]);
                        block_pixels[dst_idx + 3] = s_cast<uint8_t>(src[src_idx + 3]);
                    }
                }

                std::size_t block_offset = (by * blocks_x + bx) * 8;
                rgbcx::encode_bc1(10, dst + block_offset, block_pixels, false, false);
            }
        }

        return result;
    }

    auto compress_to_bc3(RawLoadedImage& image, bool is_srgb) -> RawLoadedImage {
        

        initialize_compressor();

        RawLoadedImage result = {};
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;
        result.format = is_srgb ? daxa::Format::BC3_SRGB_BLOCK : daxa::Format::BC3_UNORM_BLOCK;

        uint32_t blocks_x = (image.width + 3) / 4;
        uint32_t blocks_y = (image.height + 3) / 4;
        std::size_t block_count = blocks_x * blocks_y;
        std::size_t compressed_size = block_count * 16;

        result.data.resize(compressed_size);
        result.mipmaps.emplace_back(result.data.data(), compressed_size);

        const std::byte* src = image.data.data();
        std::byte* dst = result.data.data();

        for(uint32_t by = 0; by < blocks_y; ++by) {
            for(uint32_t bx = 0; bx < blocks_x; ++bx) {
                uint8_t block_pixels[16 * 4];

                for(uint32_t y = 0; y < 4; y++) {
                    for(uint32_t x = 0; x < 4; x++) {
                        uint32_t src_x = std::min(bx * 4 + x, image.width - 1);
                        uint32_t src_y = std::min(by * 4 + y, image.height - 1);
                        uint32_t src_idx = (src_y * image.width + src_x) * 4;
                        uint32_t dst_idx = (y * 4 + x) * 4;

                        block_pixels[dst_idx + 0] = s_cast<uint8_t>(src[src_idx + 0]);
                        block_pixels[dst_idx + 1] = s_cast<uint8_t>(src[src_idx + 1]);
                        block_pixels[dst_idx + 2] = s_cast<uint8_t>(src[src_idx + 2]);
                        block_pixels[dst_idx + 3] = s_cast<uint8_t>(src[src_idx + 3]);
                    }
                }

                std::size_t block_offset = (by * blocks_x + bx) * 16;
                rgbcx::encode_bc3(10, dst + block_offset, block_pixels);
            }
        }

        return result;
    }

    auto compress_to_bc5_rg(RawLoadedImage& image) -> RawLoadedImage {
        

        initialize_compressor();

        RawLoadedImage result = {};
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;
        result.format = daxa::Format::BC5_UNORM_BLOCK;

        uint32_t blocks_x = (image.width + 3) / 4;
        uint32_t blocks_y = (image.height + 3) / 4;
        std::size_t block_count = blocks_x * blocks_y;
        std::size_t compressed_size = block_count * 16;

        result.data.resize(compressed_size);
        result.mipmaps.push_back(std::span<std::byte const>(result.data.data(), compressed_size));

        const std::byte* src = image.data.data();
        std::byte* dst = result.data.data();

        for(uint32_t by = 0; by < blocks_y; by++) {
            for(uint32_t bx = 0; bx < blocks_x; bx++) {
                uint8_t block_pixels[16 * 4];

                for(uint32_t y = 0; y < 4; y++) {
                    for(uint32_t x = 0; x < 4; x++) {
                        uint32_t src_x = std::min(bx * 4 + x, image.width - 1);
                        uint32_t src_y = std::min(by * 4 + y, image.height - 1);
                        uint32_t src_idx = (src_y * image.width + src_x) * 4;
                        uint32_t dst_idx = (y * 4 + x) * 4;

                        block_pixels[dst_idx + 0] = s_cast<uint8_t>(src[src_idx + 2]);
                        block_pixels[dst_idx + 1] = s_cast<uint8_t>(src[src_idx + 1]);
                        block_pixels[dst_idx + 2] = 0;
                        block_pixels[dst_idx + 3] = 255;
                    }
                }

                std::size_t block_offset = (by * blocks_x + bx) * 16;
                rgbcx::encode_bc5(dst + block_offset, block_pixels, 0, 1, 4);
            }
        }

        return result;
    }

    auto compress_to_bc5_normal(RawLoadedImage& image) -> RawLoadedImage {
        

        initialize_compressor();

        RawLoadedImage result = {};
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;
        result.format = daxa::Format::BC5_UNORM_BLOCK;

        uint32_t blocks_x = (image.width + 3) / 4;
        uint32_t blocks_y = (image.height + 3) / 4;
        std::size_t block_count = blocks_x * blocks_y;
        std::size_t compressed_size = block_count * 16;

        result.data.resize(compressed_size);
        result.mipmaps.emplace_back(result.data.data(), compressed_size);

        const std::byte* src = image.data.data();
        std::byte* dst = result.data.data();

        for(uint32_t by = 0; by < blocks_y; by++) {
            for(uint32_t bx = 0; bx < blocks_x; bx++) {
                uint8_t block_pixels[16 * 4];

                for(uint32_t y = 0; y < 4; y++) {
                    for(uint32_t x = 0; x < 4; x++) {
                        uint32_t src_x = std::min(bx * 4 + x, image.width - 1);
                        uint32_t src_y = std::min(by * 4 + y, image.height - 1);
                        uint32_t src_idx = (src_y * image.width + src_x) * 4;
                        uint32_t dst_idx = (y * 4 + x) * 4;

                        float nx = (s_cast<float>(src[src_idx + 0]) / 255.0f) * 2.0f - 1.0f;
                        float ny = (s_cast<float>(src[src_idx + 1]) / 255.0f) * 2.0f - 1.0f;
                        float nz = (s_cast<float>(src[src_idx + 2]) / 255.0f) * 2.0f - 1.0f;

                        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                        if(len > 0.0f) {
                            nx /= len;
                            ny /= len;
                        }

                        block_pixels[dst_idx + 0] = s_cast<uint8_t>((nx * 0.5f + 0.5f) * 255.0f);
                        block_pixels[dst_idx + 1] = s_cast<uint8_t>((ny * 0.5f + 0.5f) * 255.0f);
                        block_pixels[dst_idx + 2] = 0;
                        block_pixels[dst_idx + 3] = 255;
                    }
                }

                std::size_t block_offset = (by * blocks_x + bx) * 16;
                rgbcx::encode_bc5(dst + block_offset, block_pixels, 0, 1, 4);
            }
        }

        return result;
    }

    auto compress_image(RawLoadedImage& image, TextureType texture_type) -> RawLoadedImage {

        if(image.format != daxa::Format::R8G8B8A8_UNORM && image.format != daxa::Format::R8G8B8A8_SRGB) {
            return image;
        }

        bool is_srgb = (image.format == daxa::Format::R8G8B8A8_SRGB);
        bool alpha_tested = (texture_type == TextureType::GltfAlbedo) && has_alpha_channel(image);
        RawLoadedImage image_with_mipmaps = generate_mipmaps(image, alpha_tested);

        RawLoadedImage result = {};
        result.width = image.width;
        result.height = image.height;
        result.depth = image.depth;

        daxa::Format target_format;
        switch(texture_type) {
            case TextureType::GltfAlbedo: {
                target_format = has_alpha_channel(image)
                                    ? (is_srgb ? daxa::Format::BC3_SRGB_BLOCK : daxa::Format::BC3_UNORM_BLOCK)
                                    : (is_srgb ? daxa::Format::BC1_RGBA_SRGB_BLOCK : daxa::Format::BC1_RGBA_UNORM_BLOCK);
                break;
            }

            case TextureType::GltfEmissive:
                target_format = is_srgb ? daxa::Format::BC1_RGBA_SRGB_BLOCK : daxa::Format::BC1_RGBA_UNORM_BLOCK;
                break;

            case TextureType::GltfRoughnessMetallic:
                target_format = daxa::Format::BC5_UNORM_BLOCK;
                break;

            case TextureType::GltfNormal:
                target_format = daxa::Format::BC5_UNORM_BLOCK;
                break;

            default:
                return image;
        }

        result.format = target_format;

        uint32_t current_width = image.width;
        uint32_t current_height = image.height;

        for(uint32_t mip = 0; mip < image_with_mipmaps.mipmaps.size(); ++mip) {
            RawLoadedImage mip_image = {};
            mip_image.width = current_width;
            mip_image.height = current_height;
            mip_image.depth = 1;
            mip_image.format = image.format;
            mip_image.data.resize(image_with_mipmaps.mipmaps[mip].size());
            std::memcpy(mip_image.data.data(), image_with_mipmaps.mipmaps[mip].data(), image_with_mipmaps.mipmaps[mip].size());

            RawLoadedImage compressed_mip;

            switch(texture_type) {
            case TextureType::GltfAlbedo:
                if(has_alpha_channel(mip_image)) {
                    compressed_mip = compress_to_bc3(mip_image, is_srgb);
                } else {
                    compressed_mip = compress_to_bc1(mip_image, is_srgb);
                }
                break;

            case TextureType::GltfEmissive:
                compressed_mip = compress_to_bc1(mip_image, is_srgb);
                break;

            case TextureType::GltfRoughnessMetallic:
                compressed_mip = compress_to_bc5_rg(mip_image);
                break;

            case TextureType::GltfNormal:
                compressed_mip = compress_to_bc5_normal(mip_image);
                break;

            default:
                compressed_mip = mip_image;
                break;
            }

            std::size_t prev_size = result.data.size();
            result.data.resize(prev_size + compressed_mip.data.size());
            std::memcpy(result.data.data() + prev_size, compressed_mip.data.data(), compressed_mip.data.size());
            result.mipmaps.emplace_back(result.data.data() + prev_size, compressed_mip.data.size());

            current_width = std::max(1u, current_width / 2);
            current_height = std::max(1u, current_height / 2);
        }

        return result;
    }

    */
} // namespace foundation