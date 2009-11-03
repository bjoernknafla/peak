/**
 * @file
 *
 *
 * TODO: @todo Add restrict keyword to memory functions.
 */

#include <UnitTest++.h>


#include <cstddef>

#include <peak/peak_data_alignment.h>
#include <peak/pmem.h>



SUITE(data_alignment)
{
    
    namespace 
    {
        typedef unsigned char peak_byte_t;
        typedef unsigned char peak_uint8_t;
        typedef unsigned short peak_uint16_t;

        
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
        std::size_t const main_alloc_testruns = 100;
        
        for (std::size_t i = 0; i < main_alloc_testruns; ++i) {
            
            for (std::size_t p = 1; p < 16; ++p) {

                std::size_t const alignment = (1u << p);
                for (std::size_t msize = 4; msize < 4096; msize *= 2) {
                    void *m = pmem_malloc_aligned( msize, alignment);
                    CHECK(peak_is_aligned(m, alignment));
                    pmem_free_aligned(m);
                }
            }
            
        }
    }
    
    
    
} // SUITE(data_alignment)

