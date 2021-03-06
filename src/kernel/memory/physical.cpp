#include "physical.hpp"

#include <kernel/utility/utility.hpp>
#include <kernel/vga.hpp>

using namespace kernel;

static u8*    bitmap;
static size_t bitmap_size;

static size_t mem_total = 0;
static size_t mem_avail = 0;

static mem_info_t* mem_info;
static size_t      mem_info_size;

static paddr_t KERNEL_END = 4194304;

#define _2MB  (1 << 21)
#define _32KB (1 << 15)
#define _4KB  (1 << 12)

#define ALIGN(addr, align) (addr & ~align)

#define BMP_BYTE(addr) (addr / _32KB)
#define BMP_BIT (addr) ((addr % _32KB) / _4KB)

size_t kernel::get_total_memory()
{
    return mem_total;
}

size_t kernel::get_available_memory()
{
    return mem_avail;
}

static u8 _first_free_bit(u8 byte)
{
    u8 bit = 0;
    while ((byte & 1) == 1)
    {
        byte >>= 1;
        bit++;
    }

    return bit; 
}

static void _parse_mem_info()
{
    for (int idx = 0; idx < mem_info_size; idx++)
    {
        mem_info_t* cur = &mem_info[idx];

        mem_total += cur->length;
        if (cur->type == MEM_FREE)
            mem_avail += cur->length;
    }

    bitmap_size = utility::divceil(mem_total, _32KB + 0UL);
}

static void _init_bitmap()
{
    // The bitmap will be located on the first 2 MB region
    // immediately after the kernel
    uint64_t begin = ALIGN(KERNEL_END, _2MB) + _2MB;

    bitmap = (u8*) begin;

    // Zero out bitmap
    utility::memzro(bitmap, bitmap_size);

    // Mark kernel pages as taken
    // TODO: Instead of using granularity of 2 MB for
    // bitmap, shrink it to 4 KB.
    // 2 MB can account for a total of ~68 GB of memory
    u64 kernel_usage = (begin + _2MB) / _32KB; // kernel_usage in bitmap bytes
    utility::memset(bitmap, kernel_usage, 255);
}

// TODO: actually use amt
static frame_t _find_first(size_t amt)
{
    u8 FULL = ~0;

    for (u32 idx = 0; idx < bitmap_size; idx++)
    {
        u8 byte = bitmap[idx];
        if (byte == FULL)
            continue;

        u8 bit = _first_free_bit(byte);
        return (idx * _32KB) + (bit * _4KB);
    }

    // No enough free space left, return an error
    return 0;
}


frame_t kernel::pmm_alloc(size_t amt)
{
    frame_t base = _find_first(amt);
    pmm_reserve(base, amt);
    return base;
}

void kernel::pmm_reserve(frame_t base, size_t amt)
{
    base = ALIGN(base, _4KB);

    // Reserve entire bytes first
    u32 cur_byte = BMP_BYTE(base);
    u32 bytes    = amt / 8;

    while (bytes--)
        bitmap[cur_byte++] = 255;

    // Set remaining bits
    u32 bits = amt % 8;
    u32 bit  = _first_free_bit(bitmap[cur_byte]);
    while (bits)
    {
        bitmap[cur_byte] |= (1 << bit);
        bit++;
        bits--;
    }
}

void kernel::pmm_free(frame_t base, size_t amt)
{

}

void kernel::pmm_init(mem_info_t* mem_info, size_t amt)
{
    ::mem_info      = mem_info;
    ::mem_info_size = amt;

    _parse_mem_info();
    _init_bitmap();

    // Finally, mark unusable regions from mem_info in the bitmap
    for (int idx = 0; idx < amt; idx++)
    {
        mem_info_t* cur = &mem_info[idx];
        if (cur->type != MEM_FREE)
        {
            frame_t start = cur->base;
            size_t  len   = utility::divceil(cur->length, _4KB + 0UL);

            kernel::printf("Reserving %xl: %l pages\n", start, len); 
            pmm_reserve(start, len);
        }
    }
}