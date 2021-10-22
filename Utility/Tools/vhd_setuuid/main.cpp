

#include <stdio.h>
#include <cstdint>
#include <cstdio>
#include <string>

namespace hdimage
{
    typedef struct _UUID
    {
        uint32_t u32;
        uint16_t u16_2[2];
        uint8_t u8_2[2];
        uint8_t u8_6[6];
    } UUID;

    static_assert(sizeof(UUID) == 0x10, "size mismatch");

    namespace misc
    {
        uint16_t big_endian(uint16_t value)
        {
            return
                ((value >> 0x08) & 0x00ff) |
                ((value << 0x08) & 0xff00);
        }

        uint32_t big_endian(uint32_t value)
        {
            return
                ((value >> 0x18) & 0x000000ff) |
                ((value >> 0x08) & 0x0000ff00) |
                ((value << 0x08) & 0x00ff0000) |
                ((value << 0x18) & 0xff000000);
        }

        UUID string_to_uuid(char *s)
        {
            // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
            UUID uuid{};
            sscanf_s(s, "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                &uuid.u32,
                &uuid.u16_2[0], &uuid.u16_2[1],
                &uuid.u8_2[0], &uuid.u8_2[1],
                &uuid.u8_6[0], &uuid.u8_6[1], &uuid.u8_6[2], &uuid.u8_6[3],
                &uuid.u8_6[4], &uuid.u8_6[5]);

            return uuid;
        }
    }

    typedef struct _VHD_FOOTER
    {
        char Cookie[8]; // "conectix"
        uint32_t Features;
        uint32_t FileFormatVersion;
        uint64_t DataOffset;
        uint32_t Timestamp;
        uint32_t CreatorApplication;
        uint32_t CreatorVersion;
        uint32_t CreatorHostOS;
        uint64_t OriginalSize;
        uint64_t CurrentSize;
        uint32_t DiskGeometry;
        uint32_t DiskType;
        uint32_t Checksum;
        UUID Uuid;
        uint8_t SavedState;
        uint8_t Reserved[427];
    } VHD_FOOTER;

    static_assert(sizeof(VHD_FOOTER) == 0x200, "size mismatch");

    class vhd
    {
    public:
        vhd(char *path) : path_(path), footer_cache_{}, file_(nullptr), size_(0), valid_(false), changed_(false)
        {
        }

        ~vhd()
        {
            if (file_)
                close();
        }

        bool open()
        {
            file_ = nullptr;

            fopen_s(&file_, path_.c_str(), "r+b");
            if (!file_)
                return false;

            valid_ = false;
            changed_ = false;

            fseek(file_, 0, SEEK_END);
            size_ = ftell(file_);
            fseek(file_, 0, SEEK_SET);

            if ((size_ & 0x1ff) != 0x1ff &&
                (size_ & 0x1ff) != 0)
            {
                // opened but invalid
                return true;
            }

            long long footer_offset = get_footer_offset(size_);
            if (!footer_offset)
            {
                // opened but invalid
                return true;
            }

            _fseeki64(file_, footer_offset, SEEK_SET);
            fread(&footer_cache_, sizeof(footer_cache_), 1, file_);

            if (!validate_footer(&footer_cache_))
            {
                // opened but invalid
                return true;
            }

            valid_ = true;

            return true;
        }

        int close()
        {
            if (!file_)
                return -1;

            int error = 0;
            if (valid_ && changed_)
            {
                set_footer_checksum(&footer_cache_);
                _fseeki64(file_, get_footer_offset(size_), SEEK_SET);

                if (!fwrite(&footer_cache_, sizeof(footer_cache_), 1, file_))
                {
                    printf("failed to write image\n");
                    error = -1;
                }

                fflush(file_);
                changed_ = false;
            }

            fclose(file_);

            printf("file closed\n");

            file_ = nullptr;
            valid_ = false;

            return error;
        }

        bool validate_footer(VHD_FOOTER *footer)
        {
            // only accepts fixed disk (2)
            if (misc::big_endian(footer->DiskType) != 2)
                return false;

            if ((misc::big_endian(footer->FileFormatVersion) & 0xffff0000) != 0x00010000)
                return false;

            VHD_FOOTER temp = *footer;
            temp.Checksum = 0;

            uint32_t checksum = checksum_calc(~0, reinterpret_cast<uint8_t *>(&temp), sizeof(*footer));
            if (checksum != misc::big_endian(footer->Checksum))
                return false;

            return true;
        }

        long long get_footer_offset(long long size)
        {
            long long footer_offset = 0;

            if ((size_ & 0x1ff) == 0)
            {
                if (size_ > 0x200)
                    footer_offset = size_ - 0x200;
            }
            else if ((size_ & 0x1ff) == 0x1ff)
            {
                footer_offset = size_ & ~0x1ff;
            }

            return footer_offset;
        }

        void set_footer_checksum(VHD_FOOTER *footer)
        {
            footer->Checksum = 0;
            footer->Checksum = checksum_calc(~0, reinterpret_cast<uint8_t *>(footer), sizeof(*footer));
            footer->Checksum = misc::big_endian(footer->Checksum);
        }

        void set_uuid(UUID uuid)
        {
            UUID uuid_be = uuid;
            //uuid_be.u32 = misc::big_endian(uuid_be.u32);
            //uuid_be.u16_2[0] = misc::big_endian(uuid_be.u16_2[0]);
            //uuid_be.u16_2[1] = misc::big_endian(uuid_be.u16_2[1]);

            footer_cache_.Uuid = uuid_be;
            changed_ = true;
        }

        bool valid() const
        {
            return file_ && valid_;
        }

    private:
        uint32_t checksum_calc(uint32_t checksum, uint8_t *buf, size_t size)
        {
            checksum = ~checksum;

            for (size_t i = 0; i < size; i++)
            {
                checksum += buf[i];
            }

            return ~checksum;
        }

        std::string path_;
        VHD_FOOTER footer_cache_;
        FILE *file_;
        unsigned long long size_;
        bool valid_;
        bool changed_;
    };
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf(
            "argument is missing\n"
            "%s [vhd_path] [uuid]\n\n"
            "example:\n"
            "%s \"D:\\test.vhd\" 12345678-9abc-def0-baad-deadfbeef00d\n\n",
            argv[0], argv[0]);

        return -2;
    }

    char *path = argv[1];
    hdimage::UUID uuid = hdimage::misc::string_to_uuid(argv[2]);

    printf("opening VHD image `%s'...\n", path);

    hdimage::vhd image(path);

    if (!image.open())
    {
        printf("failed to open VHD image `%s'\n", path);
        return -1;
    }

    if (!image.valid())
    {
        printf("invalid VHD image\n");
        image.close();
        return -1;
    }

    printf("setting UUID\n");
    image.set_uuid(uuid);

    return image.close();
}

