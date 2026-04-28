#include <ktx.h>
#include <cmath>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>

struct BrightestSample
{
    int face = -1;
    int x = 0, y = 0;
    float lum = 0.0f;
    float r = 0, g = 0, b = 0; 
};

static float SrgbToLinear(float c)
{
    if (c <= 0.04045f) return c / 12.92f;
    return std::pow((c + 0.055f) / 1.055f, 2.4f);
}

BrightestSample FindBrightestTexel_KTX2_RGBA8Cubemap(
    const char* ktx2Path,
    bool sourceIsSRGB = true,
    uint32_t mipToScan = 3 
)
{
    BrightestSample best;

    ktxTexture2* tex2 = nullptr;
    KTX_error_code ec = ktxTexture2_CreateFromNamedFile(
        ktx2Path,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &tex2
    );
    if (ec != KTX_SUCCESS || !tex2) return best;

    ktxTexture* tex = ktxTexture(tex2);

    if (tex->numFaces != 6) { ktxTexture_Destroy(tex); return best; }
    if (mipToScan >= tex->numLevels) mipToScan = tex->numLevels - 1;

    const uint32_t w = __max(1u, tex->baseWidth >> mipToScan);
    const uint32_t h = __max(1u, tex->baseHeight >> mipToScan);


    const uint8_t* imageData = reinterpret_cast<const uint8_t*>(tex->pData);

    float maxLum = -std::numeric_limits<float>::infinity();

    for (uint32_t face = 0; face < 6; ++face)
    {
        ktx_size_t offset = 0;
        ec = ktxTexture_GetImageOffset(tex, mipToScan, 0, face, &offset);
        if (ec != KTX_SUCCESS) continue;

        const uint8_t* facePixels = imageData + offset;

        const uint32_t strideBytes = w * 4;

        for (uint32_t y = 0; y < h; ++y)
        {
            const uint8_t* row = facePixels + y * strideBytes;
            for (uint32_t x = 0; x < w; ++x)
            {
                const uint8_t* px = row + x * 4;

                float r = px[0] / 255.0f;
                float g = px[1] / 255.0f;
                float b = px[2] / 255.0f;

                if (sourceIsSRGB) {
                    r = SrgbToLinear(r);
                    g = SrgbToLinear(g);
                    b = SrgbToLinear(b);
                }

                float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;

                if (lum > maxLum)
                {
                    maxLum = lum;
                    best.face = (int)face;
                    best.x = (int)x;
                    best.y = (int)y;
                    best.lum = lum;
                    best.r = r; best.g = g; best.b = b;
                }
            }
        }
    }

    ktxTexture_Destroy(tex);
    return best;
}
static float Luminance(float r, float g, float b)
{
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}
glm::vec3 AccumulateSunTintRing(
    const uint8_t* facePixels,
    uint32_t w, uint32_t h,
    int cx, int cy,
    int innerR, int outerR,
    bool srcIsSRGB)
{
    glm::vec3 sum(0.0f);
    float wsum = 0.0f;

    int inner2 = innerR * innerR;
    int outer2 = outerR * outerR;

    for (int dy = -outerR; dy <= outerR; ++dy)
        for (int dx = -outerR; dx <= outerR; ++dx)
        {
            int r2 = dx * dx + dy * dy;
            if (r2 < inner2 || r2 > outer2) continue;

            int x = cx + dx, y = cy + dy;
            if (x < 0 || y < 0 || x >= (int)w || y >= (int)h) continue;

            const uint8_t* px = facePixels + (y * w + x) * 4;

            if (px[0] == 255 && px[1] == 255 && px[2] == 255) continue;

            float r = px[0] / 255.0f;
            float g = px[1] / 255.0f;
            float b = px[2] / 255.0f;

            if (srcIsSRGB) { r = SrgbToLinear(r); g = SrgbToLinear(g); b = SrgbToLinear(b); }

            float lum = Luminance(r, g, b);
            float wgt = lum * lum;      

            sum += glm::vec3(r, g, b) * wgt;
            wsum += wgt;
        }
    glm::vec3 out;
    if (wsum <= 0.0f) out = glm::vec3(1.0f); 
    out = sum / wsum;
    std::cout << "out: " << out.x << "," << out.y << "," << out.z << std::endl;
    return out;
}

glm::vec3 FaceUVToDir(int face, float u, float v)
{
    // flip v because image y grows downward
    float s = u;
    float t = -v;
    glm::vec3 f;
    switch (face)
    {
    case 0: f = glm::normalize(glm::vec3(1, t, -s));  break;
    case 1: f = glm::normalize(glm::vec3(-1, t, s));  break;
    case 2: f = glm::normalize(glm::vec3(s, 1, -t));  break;
    case 3: f = glm::normalize(glm::vec3(s, -1, t));  break;
    case 4: f = glm::normalize(glm::vec3(s, t, 1));  break;
    case 5: f = glm::normalize(glm::vec3(-s, t, -1)); break;
    default:f = glm::vec3(0, 0, 1);  break;
    }
    std::cout << "FaceUVToDir: " << f.x << "," << f.y << "," << f.z << std::endl;
    return f;
}

struct CubemapMipView
{
    uint32_t w = 0, h = 0;
    uint32_t rowPitch = 0;        
    std::array<const uint8_t*, 6> facePixels{}; 
};

CubemapMipView GetCubemapMipPixelsRGBA8(const char* path, uint32_t mip)
{
    CubemapMipView out{};

    ktxTexture2* tex2 = nullptr;
    KTX_error_code ec = ktxTexture2_CreateFromNamedFile(
        path,
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &tex2
    );
    if (ec != KTX_SUCCESS || !tex2) return out;

    ktxTexture* tex = ktxTexture(tex2);

    if (tex->numFaces != 6) { ktxTexture_Destroy(tex); return out; }
    if (mip >= tex->numLevels) mip = tex->numLevels - 1;

    out.w = std::max(1u, tex->baseWidth >> mip);
    out.h = std::max(1u, tex->baseHeight >> mip);

    out.rowPitch = (uint32_t)ktxTexture_GetRowPitch(tex, mip);

    const uint8_t* base = reinterpret_cast<const uint8_t*>(tex->pData);

    for (uint32_t face = 0; face < 6; ++face)
    {
        ktx_size_t offset = 0;
        ec = ktxTexture_GetImageOffset(tex, mip, 0, face, &offset);
        if (ec != KTX_SUCCESS) { out.facePixels[face] = nullptr; continue; }

        out.facePixels[face] = base + offset;
    }

    return out; 
}