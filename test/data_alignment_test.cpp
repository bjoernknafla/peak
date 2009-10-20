/**
 * @file
 *
 *
 * TODO: @todo Add restrict keyword to memory functions.
 */

#include <UnitTest++.h>



#include <assert.h>


#if defined(PEAK_USE_WIN)
#   include <windows.h>
#   include <malloc.h>
#elif defined(PEAK_USE_STDC99)
#   include <stdint.h>
#else
#   error Unsupported platform.
#endif

// See http://www.gamasutra.com/view/feature/3942/data_alignment_part_1.php
// See http://www.gamasutra.com/view/feature/3975/data_alignment_part_2_objects_on_.php
// See http://developer.amd.com/documentation/articles/pages/1213200696.aspx
// See Steven Goodwin, Cross-Platform Game Programming, Charles River Media, 2005
//
// SSE vector instructions work on 16byte aligned data
// Data alignment also important to enable atomicity of memory access
//
// Static Data
// Specifying the alignment of a member var forces the compiler to align it
// relative to the start of the struct it belongs to.
// The alignment of a struct has to be a multiple of the least common multiple
// of the alignment of its member fields.
//
SUITE(data_alignment)
{
    
    namespace 
    {
        
        // Alignment for static / global variables (NOT for data on the heap
        // or on the stack (arguments or function stack).
#if defined(PEAK_USE_MSVC)
#   define PEAK_STATIC_DATA_ALIGN(declaration, alignment) __declspec(align(alignment))declaration
#   define PEAK_STATIC_TYPE_ALIGN_BEGIN(alignment) __declspec(align(alignment))
#   define PEAK_STATIC_TYPE_ALIGN_END(alignment)
#elif defined(PEAK_USE_GCC)
#   define PEAK_STATIC_DATA_ALIGN(declaration, alignment) declaration __attribute__((aligned(alignment)))
#   define PEAK_STATIC_TYPE_ALIGN_BEGIN(alignment)
#   define PEAK_STATIC_TYPE_ALIGN_END(alignment) __attribute__((aligned(alignment)))
#else
#   error Unsupported platform.
#endif
        
        
        
        
        typedef unsigned char peak_byte_t;
        typedef unsigned char peak_uint8_t;
        typedef unsigned short peak_uint16_t;
        
        
        
        int peak_is_power_of_two_or_zero(size_t tested_byte_alignment)
        {
            return (0 == (tested_byte_alignment 
                          & (tested_byte_alignment - 1)));
        }
        
        
        int peak_is_power_of_two(size_t tested_byte_alignment)
        {
            size_t not_zero = (0 != tested_byte_alignment);
            size_t zero_or_power_of_two = (0 == (tested_byte_alignment 
                                                       & (tested_byte_alignment - 1)));
            return not_zero && zero_or_power_of_two;
        }
        
        // byte_alignment must be a power of two and therefore mustn't be zero.
        int peak_is_aligned(void *pointer_to_check,
                            size_t byte_alignment)
        {
            assert(0 != byte_alignment);
            assert(peak_is_power_of_two(byte_alignment));
            
            return (((uintptr_t)pointer_to_check) & (byte_alignment-1)) == 0;
        }
        
#if defined(PEAK_USE_MSVC)
        void *pmem_malloc_aligned(size_t bytes, size_t alignment)
        {
            // TODO: @todo Decide if to use _aligned_malloc_dbg in debug mode.
            assert(0 != peak_is_power_of_two(alignment));
            assert(peak_is_power_of_two(alignment));
            assert(bytes <= _HEAP_MAXREQ);
            
            return _aligned_malloc(bytes, alignment);
        }
        
        void pmem_free_aligned(void *aligned_pointer)
        {
            // TODO: @todo Decide if to use _aligned_free_dbg in debug mode.
            _aligned_free(aligned_pointer);
        }
        
        
#else
        // Returns a pointer to a memory area of size bytes that is aligned
        // to alignment or NULL if no memory could be allocated.
        // 
        // Might use malloc internally.
        //
        // Use to allocate a lot of memory and not for individual allocation
        // of small types as memory is reserved to help implement free.
        //
        // alignment must be a power of two and greater than 0.
        // free memory allocated with this function via pmem_free_aligned.
        // The byte alignment can be at max 65533 (keep it below pow(2, 12) to
        // play it safe).
        //
        // If no memory could be allocated NULL is returned and
        // errno is set to ENOMEM.
        // If one of the arguments is bad (e.g. bad alignment argument) NULL
        // might be returned and errno is set to EINVAL. This is a programmer 
        // error and possibly an error handler is called or in debug mode 
        // an assertion triggers.
        void *pmem_malloc_aligned(size_t bytes, size_t alignment)
        {
            assert(0 < alignment);
            // (2^16 - 1) - 2 to reserve space to store how many pointers to  
            // jump back from the aligned pointer to the original raw pointer.
            assert(alignment <= 65533);
            assert(peak_is_power_of_two(alignment));
            
            // Add 2 to reserve memory to store the byte shift needed to go from
            // the returned aligned memory pointer to the original raw memory.
            // 2 bytes allow handling of huge packed and alignment-needing
            // vector data.
            ptrdiff_t const max_alignment = alignment - 1;
            ptrdiff_t const max_shift = 2 + max_alignment;
            
            void *raw_ptr = malloc( bytes + max_shift );
            void *aligned_ptr = NULL;
            if (NULL != raw_ptr) {
                
                void *max_shift_ptr = (peak_byte_t*)raw_ptr + max_shift;
                
                aligned_ptr = (void*)(((uintptr_t)max_shift_ptr) & (~max_alignment));
                
                // Store bytes to jump to original ptr beginning directly in front
                // of aligned data.
                *(((peak_uint16_t*)aligned_ptr)-1) = (peak_uint16_t)(((peak_byte_t*)aligned_ptr) - ((peak_byte_t*)raw_ptr));
            }
            
            return aligned_ptr;
        }
        
        // Might use free internally
        //
        // Only free memory allocated via pmem_malloc_aligned with this func.
        void pmem_free_aligned(void *aligned_pointer)
        {
            // TODO: @todo Profile and decide if to use a method that stores
            //             the address of the raw pointer directly in front of
            //             the aligned memory which might use more memory but
            //             also might be faster to use as the pointer might be
            //             naturally aligned to access it.
            
            if (NULL != aligned_pointer) {
                
                // How many bytes was the original pointer address shifted
                // to be correctly aligned?
                peak_uint16_t const alignment_shift = *(((peak_uint16_t*)aligned_pointer) - 1);
                
                // Extract the original pointer address and free it.
                void *original_ptr = ((peak_byte_t*)aligned_pointer) - alignment_shift;
                
                free(original_ptr);
            }
        }

#endif
        
        
    } // anonymous namespace
    
    
    TEST(peak_type_sizes)
    {
        CHECK(sizeof(peak_byte_t) * CHAR_BIT == 8);
        CHECK(sizeof(peak_uint8_t) * CHAR_BIT == 8);
        CHECK(sizeof(peak_uint16_t) * CHAR_BIT == 16);
        
        CHECK(((peak_byte_t)0)-1 == ~((peak_byte_t)0));
        CHECK(((peak_uint8_t)0)-1 == ~((peak_uint8_t)0));
        CHECK(((peak_uint16_t)0)-1 == ~((peak_uint16_t)0));
    }
    
    
    TEST(align)
    {
        CHECK_EQUAL(0, peak_is_power_of_two(0));
        CHECK_EQUAL(0, peak_is_power_of_two(3));
        CHECK_EQUAL(0, peak_is_power_of_two(8+4));
        
        CHECK_EQUAL(1, peak_is_power_of_two_or_zero(0));
        CHECK_EQUAL(0, peak_is_power_of_two_or_zero(3));
        CHECK_EQUAL(0, peak_is_power_of_two_or_zero(8+4));
        
        for (unsigned int i= 0; i < 16; ++i) {
            
            unsigned int value_to_check = (1 << i);
            
            CHECK_EQUAL(1, peak_is_power_of_two(value_to_check));
            CHECK_EQUAL(1, peak_is_power_of_two_or_zero(value_to_check));
        }
        
    }
    
    
    TEST(pmem_malloc_aligned)
    {
        size_t const main_alloc_testruns = 100;
        
        for (size_t i = 0; i < main_alloc_testruns; ++i) {
            
            for (size_t p = 1; p < 16; ++p) {

                size_t const alignment = (1u << p);
                for (size_t msize = 4; msize < 4096; msize *= 2) {
                    void *m = pmem_malloc_aligned( msize, alignment);
                    CHECK(peak_is_aligned(m, alignment));
                    pmem_free_aligned(m);
                }
            }
            
        }
    }
    
    
    
} // SUITE(data_alignment)

