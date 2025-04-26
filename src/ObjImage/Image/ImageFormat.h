#pragma once

#include "D3DFormat.h"
#include "DxgiFormat.h"

#include <cstddef>
#include <cstdint>

enum class ImageFormatId
{
    UNKNOWN = -1,
    R8_G8_B8,
    B8_G8_R8_X8,
    R8_G8_B8_A8,
    B8_G8_R8_A8,
    A8,
    R16_G16_B16_A16_FLOAT,
    R8,
    R8_A8,
    BC1,
    BC2,
    BC3,
    BC4,
    BC5,

    MAX
};

enum class ImageFormatType
{
    UNKNOWN,
    UNSIGNED,
    BLOCK_COMPRESSED
};

class ImageFormatUnsigned;
class ImageFormatBlockCompressed;

class ImageFormat
{
    ImageFormatId m_id;
    oat::D3DFORMAT m_d3d_format;
    oat::DXGI_FORMAT m_dxgi_format;

protected:
    ImageFormat(ImageFormatId id, oat::D3DFORMAT d3dFormat, oat::DXGI_FORMAT dxgiFormat);

public:
    virtual ~ImageFormat() = default;

    [[nodiscard]] ImageFormatId GetId() const;
    [[nodiscard]] oat::D3DFORMAT GetD3DFormat() const;
    [[nodiscard]] oat::DXGI_FORMAT GetDxgiFormat() const;

    [[nodiscard]] virtual ImageFormatType GetType() const = 0;
    [[nodiscard]] virtual size_t GetPitch(unsigned mipLevel, unsigned width) const = 0;
    [[nodiscard]] virtual size_t GetSizeOfMipLevel(unsigned mipLevel, unsigned width, unsigned height, unsigned depth) const = 0;

    static const ImageFormatUnsigned FORMAT_R8_G8_B8;
    static const ImageFormatUnsigned FORMAT_B8_G8_R8_X8;
    static const ImageFormatUnsigned FORMAT_R8_G8_B8_A8;
    static const ImageFormatUnsigned FORMAT_B8_G8_R8_A8;
    static const ImageFormatUnsigned FORMAT_A8;
    static const ImageFormatUnsigned FORMAT_R16_G16_B16_A16_FLOAT; // TODO: Float not unsigned
    static const ImageFormatUnsigned FORMAT_R8;
    static const ImageFormatUnsigned FORMAT_R8_A8;
    static const ImageFormatBlockCompressed FORMAT_BC1;
    static const ImageFormatBlockCompressed FORMAT_BC2;
    static const ImageFormatBlockCompressed FORMAT_BC3;
    static const ImageFormatBlockCompressed FORMAT_BC4;
    static const ImageFormatBlockCompressed FORMAT_BC5;
    static const ImageFormat* const ALL_FORMATS[static_cast<unsigned>(ImageFormatId::MAX)];
};

class ImageFormatUnsigned final : public ImageFormat
{
public:
    unsigned m_bits_per_pixel;
    unsigned m_r_offset;
    unsigned m_r_size;
    unsigned m_g_offset;
    unsigned m_g_size;
    unsigned m_b_offset;
    unsigned m_b_size;
    unsigned m_a_offset;
    unsigned m_a_size;

    ImageFormatUnsigned(ImageFormatId id,
                        oat::D3DFORMAT d3dFormat,
                        oat::DXGI_FORMAT dxgiFormat,
                        unsigned bitsPerPixel,
                        unsigned rOffset,
                        unsigned rSize,
                        unsigned gOffset,
                        unsigned gSize,
                        unsigned bOffset,
                        unsigned bSize,
                        unsigned aOffset,
                        unsigned aSize);

    [[nodiscard]] ImageFormatType GetType() const override;
    [[nodiscard]] size_t GetPitch(unsigned mipLevel, unsigned width) const override;
    [[nodiscard]] size_t GetSizeOfMipLevel(unsigned mipLevel, unsigned width, unsigned height, unsigned depth) const override;

    [[nodiscard]] bool HasR() const;
    [[nodiscard]] bool HasG() const;
    [[nodiscard]] bool HasB() const;
    [[nodiscard]] bool HasA() const;
};

class ImageFormatBlockCompressed final : public ImageFormat
{
public:
    unsigned m_block_size;
    unsigned m_bits_per_block;

    ImageFormatBlockCompressed(ImageFormatId id, oat::D3DFORMAT d3dFormat, oat::DXGI_FORMAT dxgiFormat, unsigned blockSize, unsigned bitsPerBlock);

    [[nodiscard]] ImageFormatType GetType() const override;
    [[nodiscard]] size_t GetPitch(unsigned mipLevel, unsigned width) const override;
    [[nodiscard]] size_t GetSizeOfMipLevel(unsigned mipLevel, unsigned width, unsigned height, unsigned depth) const override;
};
