//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>
#include <stdexcept>
#include <sstream>

#include "bit_array.h"

//================================================================================================================================
//=> - GLOBALS -
//================================================================================================================================

#define TEST_FILED "*** TEST FAILED: "
#define TEST_PASSED "*** TEST PASSED: "

//================================================================================================================================
//=> - Tester_BitArray32 class -
//================================================================================================================================

class Tester_BitArray32 {
    
public:

    void test_zero_initialization () {
        BitArray32 ba;
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            int result = ba.get_bit(i);
            if (result != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Zero initialization");
        verify_serialize_deserialize(ba, "Zero initialization");
    }

    void test_zero_initialization_explicit () {
        BitArray32 ba(0x00000000);
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            int result = ba.get_bit(i);
            if (result != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Zero initialization explicit");
        verify_serialize_deserialize(ba, "Zero initialization explicit");
    }

    void run_test_no_crash_limits () {
        bool outcome = true;
        try {
            BitArray32 ba;
            ba.set_bit(32);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits");

        try {
            BitArray32 ba;
            ba.set_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits explicit");
    }

    void run_test_bit_masks_validation () {
        uint32_t mask = 1;
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            if (BitArray32::masks[i] != mask) {
                outcome = false;
            }
            mask *= 2;
        }
        test_result(outcome, "Bit masks validation");
    }
    
    void run_test_alternate_bits () {
        bool outcome = true;
        BitArray32 ba;
        for (int i = 0; i < 32; i++) {
            if (i % 2 == 1) {
                ba.set_bit(i);
            } else {
                ba.clear_bit(i);
            }
        }
        for (int i = 0; i < 32; i++) {
            int result = ba.get_bit(i);
            if (result != (i % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Alternate bits");
        verify_serialize_deserialize(ba, "Alternate bits");
    }

    void run_test_bit_mapping () {
        BitArray32 instances[32];
        for (int i = 0; i < 32; i++) {
            instances[i].set_bit(i);
        }
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            for (int j = i + 1; j < 32; j++) {
                if (instances[i].bits == instances[j].bits) {
                    outcome = false;
                }
            }
        }
        test_result(outcome, "Bit mapping uniqueness");
        for (int i = 0; i < 32; i++) {
            verify_serialize_deserialize(instances[i], "Bit mapping");
        }
    }

    void run_test_single_bit_set () {
        for (int bit_idx = 0; bit_idx < 32; bit_idx++) {
            BitArray32 ba;
            ba.set_bit(bit_idx);
            int count = 0;
            for (int i = 0; i < 32; i++) {
                count += ba.get_bit(i);
            }
            test_result(count == 1, "Single bit set: index: ", bit_idx);
            verify_serialize_deserialize(ba, "Single bit set");
        }
    }

    void run_test_clear_bit () {
        BitArray32 ba;
        for (int i = 0; i < 32; i++) {
            ba.set_bit(i);
        }
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            for (int j = 0; j < 32; j++) {
                if (j <= i) {
                    if (ba.get_bit(j) != 0) {
                        outcome = false;
                    }
                } else {
                    if (ba.get_bit(j) != 1) {
                        outcome = false;
                    }
                }
            }
        }
        test_result(outcome, "Clear bit");
        verify_serialize_deserialize(ba, "Clear bit");
    }

    void run_test_clear_bit_already_clear () {
        BitArray32 ba;
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Clear bit already clear");
        verify_serialize_deserialize(ba, "Clear bit already clear");
    }

    void run_test_constructor_non_zero () {
        bool outcome = true;
        BitArray32 ba1(0xFFFFFFFF);
        for (int i = 0; i < 32; i++) {
            if (ba1.get_bit(i) != 1) {
                outcome = false;
            }
        }
        BitArray32 ba2(0xAAAAAAAA);
        for (int i = 0; i < 32; i++) {
            if (ba2.get_bit(i) != (i % 2)) {
                outcome = false;
            }
        }
        BitArray32 ba3(0x55555555);
        for (int i = 0; i < 32; i++) {
            if (ba3.get_bit(i) != ((i + 1) % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Constructor non-zero");
        verify_serialize_deserialize(ba1, "Constructor non-zero");
        verify_serialize_deserialize(ba2, "Constructor non-zero");
        verify_serialize_deserialize(ba3, "Constructor non-zero");
    }

    void run_test_get_bit_out_of_bounds () {
        BitArray32 ba;
        bool outcome = true;
        if (ba.get_bit(-1) != 0) {
            outcome = false;
        }
        if (ba.get_bit(32) != 0) {
            outcome = false;
        }
        if (ba.get_bit(100) != 0) {
            outcome = false;
        }
        test_result(outcome, "Get bit out of bounds");
    }

    void run_test_clear_bit_out_of_bounds () {
        bool outcome = true;
        try {
            BitArray32 ba;
            ba.clear_bit(32);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds");

        try {
            BitArray32 ba;
            ba.clear_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds negative");
    }

    void run_test_toggle_behavior () {
        bool outcome = true;
        BitArray32 ba;
        for (int i = 0; i < 32; i++) {
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
        }
        test_result(outcome, "Toggle behavior");
        verify_serialize_deserialize(ba, "Toggle behavior");
    }

    void print_test_result () {
        printf("*** SUMMARY: %d tests, %d passed\n", test_count, test_success);
    }

private:
    void test_result (bool outcome, const char *test_name, int value) {
        std::string test_name_str = std::string(test_name) + std::to_string(value);
        test_result(outcome, test_name_str.c_str());
    }

    void test_result (bool outcome, const char *test_name) {
        if (outcome) {
            test_success++;
            if (verbose) {
                printf(TEST_PASSED "%s\n", test_name);
            }
        } else {
            printf(TEST_FILED "%s\n", test_name);
        }
        test_count++;
    }

    void verify_serialize_deserialize (const BitArray32& ba, const char* test_name) {
        std::stringstream ss;
        ba.serialize(ss);
        BitArray32 ba_deserialized = BitArray32::deserialize(ss);
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            if (ba.get_bit(i) != ba_deserialized.get_bit(i)) {
                outcome = false;
            }
        }
        std::string ser_test_name = std::string(test_name) + " serialize/deserialize";
        test_result(outcome, ser_test_name.c_str());
    }

    int test_count = 0;
    int test_success = 0;
    bool verbose = true;
};

//================================================================================================================================
//=> - Tester_BitArrayCL32 class -
//================================================================================================================================

class Tester_BitArrayCL32 {
    
public:

    void test_zero_initialization () {
        BitArrayCL ba(32);
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            int result = ba.get_bit(i);
            if (result != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Zero initialization");
    }

    void run_test_no_crash_limits () {
        bool outcome = true;
        try {
            BitArrayCL ba(32);
            ba.set_bit(32);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits");

        try {
            BitArrayCL ba(32);
            ba.set_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits explicit");
    }
    
    void run_test_alternate_bits () {
        bool outcome = true;
        BitArrayCL ba(32);
        for (int i = 0; i < 32; i++) {
            if (i % 2 == 1) {
                ba.set_bit(i);
            } else {
                ba.clear_bit(i);
            }
        }
        for (int i = 0; i < 32; i++) {
            int result = ba.get_bit(i);
            if (result != (i % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Alternate bits");
    }

    void run_test_bit_mapping () {
        BitArrayCL* instances[32];
        for (int i = 0; i < 32; i++) {
            instances[i] = new BitArrayCL(32);
            instances[i]->set_bit(i);
        }
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            for (int j = i + 1; j < 32; j++) {
                bool same = true;
                for (int k = 0; k < 32; k++) {
                    if (instances[i]->get_bit(k) != instances[j]->get_bit(k)) {
                        same = false;
                        break;
                    }
                }
                if (same) {
                    outcome = false;
                }
            }
        }
        for (int i = 0; i < 32; i++) {
            delete instances[i];
        }
        test_result(outcome, "Bit mapping uniqueness");
    }

    void run_test_single_bit_set () {
        for (int bit_idx = 0; bit_idx < 32; bit_idx++) {
            BitArrayCL ba(32);
            ba.set_bit(bit_idx);
            int count = 0;
            for (int i = 0; i < 32; i++) {
                count += ba.get_bit(i);
            }
            test_result(count == 1, "Single bit set: index: ", bit_idx);
        }
    }

    void run_test_clear_bit () {
        BitArrayCL ba(32);
        for (int i = 0; i < 32; i++) {
            ba.set_bit(i);
        }
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            for (int j = 0; j < 32; j++) {
                if (j <= i) {
                    if (ba.get_bit(j) != 0) {
                        outcome = false;
                    }
                } else {
                    if (ba.get_bit(j) != 1) {
                        outcome = false;
                    }
                }
            }
        }
        test_result(outcome, "Clear bit");
    }

    void run_test_clear_bit_already_clear () {
        BitArrayCL ba(32);
        bool outcome = true;
        for (int i = 0; i < 32; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Clear bit already clear");
    }

    void run_test_constructor_non_zero () {
        bool outcome = true;
        BitArrayCL ba1(32);
        for (int i = 0; i < 32; i++) {
            ba1.set_bit(i);
        }
        for (int i = 0; i < 32; i++) {
            if (ba1.get_bit(i) != 1) {
                outcome = false;
            }
        }
        BitArrayCL ba2(32);
        for (int i = 0; i < 32; i++) {
            if (i % 2 == 1) {
                ba2.set_bit(i);
            }
        }
        for (int i = 0; i < 32; i++) {
            if (ba2.get_bit(i) != (i % 2)) {
                outcome = false;
            }
        }
        BitArrayCL ba3(32);
        for (int i = 0; i < 32; i++) {
            if (i % 2 == 0) {
                ba3.set_bit(i);
            }
        }
        for (int i = 0; i < 32; i++) {
            if (ba3.get_bit(i) != ((i + 1) % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Constructor non-zero");
    }

    void run_test_get_bit_out_of_bounds () {
        BitArrayCL ba(32);
        bool outcome = true;
        if (ba.get_bit(-1) != 0) {
            outcome = false;
        }
        if (ba.get_bit(32) != 0) {
            outcome = false;
        }
        if (ba.get_bit(100) != 0) {
            outcome = false;
        }
        test_result(outcome, "Get bit out of bounds");
    }

    void run_test_clear_bit_out_of_bounds () {
        bool outcome = true;
        try {
            BitArrayCL ba(32);
            ba.clear_bit(32);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds");

        try {
            BitArrayCL ba(32);
            ba.clear_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds negative");
    }

    void run_test_toggle_behavior () {
        bool outcome = true;
        BitArrayCL ba(32);
        for (int i = 0; i < 32; i++) {
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
        }
        test_result(outcome, "Toggle behavior");
    }

    void print_test_result () {
        printf("*** SUMMARY: %d tests, %d passed\n", test_count, test_success);
    }

private:
    void test_result (bool outcome, const char *test_name, int value) {
        std::string test_name_str = std::string(test_name) + std::to_string(value);
        test_result(outcome, test_name_str.c_str());
    }

    void test_result (bool outcome, const char *test_name) {
        if (outcome) {
            test_success++;
            if (verbose) {
                printf(TEST_PASSED "%s\n", test_name);
            }
        } else {
            printf(TEST_FILED "%s\n", test_name);
        }
        test_count++;
    }

    int test_count = 0;
    int test_success = 0;
    bool verbose = true;
};

//================================================================================================================================
//=> - Tester_BitArrayCL96 class -
//================================================================================================================================

class Tester_BitArrayCL96 {
    
public:

    void test_zero_initialization () {
        BitArrayCL ba(96);
        bool outcome = true;
        for (int i = 0; i < 96; i++) {
            int result = ba.get_bit(i);
            if (result != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Zero initialization");
        verify_serialize_deserialize(ba, "Zero initialization");
    }

    void run_test_no_crash_limits () {
        bool outcome = true;
        try {
            BitArrayCL ba(96);
            ba.set_bit(96);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits");

        try {
            BitArrayCL ba(96);
            ba.set_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "No crash limits explicit");
    }
    
    void run_test_alternate_bits () {
        bool outcome = true;
        BitArrayCL ba(96);
        for (int i = 0; i < 96; i++) {
            if (i % 2 == 1) {
                ba.set_bit(i);
            } else {
                ba.clear_bit(i);
            }
        }
        for (int i = 0; i < 96; i++) {
            int result = ba.get_bit(i);
            if (result != (i % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Alternate bits");
        verify_serialize_deserialize(ba, "Alternate bits");
    }

    void run_test_single_bit_set () {
        int test_positions[] = {0, 31, 32, 63, 64, 95};
        for (int idx = 0; idx < 6; idx++) {
            int bit_idx = test_positions[idx];
            BitArrayCL ba(96);
            ba.set_bit(bit_idx);
            int count = 0;
            for (int i = 0; i < 96; i++) {
                count += ba.get_bit(i);
            }
            test_result(count == 1, "Single bit set: index: ", bit_idx);
        }
    }

    void run_test_clear_bit () {
        BitArrayCL ba(96);
        for (int i = 0; i < 96; i++) {
            ba.set_bit(i);
        }
        bool outcome = true;
        for (int i = 0; i < 96; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            for (int j = 0; j < 96; j++) {
                if (j <= i) {
                    if (ba.get_bit(j) != 0) {
                        outcome = false;
                    }
                } else {
                    if (ba.get_bit(j) != 1) {
                        outcome = false;
                    }
                }
            }
        }
        test_result(outcome, "Clear bit");
    }

    void run_test_clear_bit_already_clear () {
        BitArrayCL ba(96);
        bool outcome = true;
        for (int i = 0; i < 96; i++) {
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Clear bit already clear");
    }

    void run_test_constructor_non_zero () {
        bool outcome = true;
        BitArrayCL ba1(96);
        for (int i = 0; i < 96; i++) {
            ba1.set_bit(i);
        }
        for (int i = 0; i < 96; i++) {
            if (ba1.get_bit(i) != 1) {
                outcome = false;
            }
        }
        BitArrayCL ba2(96);
        for (int i = 0; i < 96; i++) {
            if (i % 2 == 1) {
                ba2.set_bit(i);
            }
        }
        for (int i = 0; i < 96; i++) {
            if (ba2.get_bit(i) != (i % 2)) {
                outcome = false;
            }
        }
        BitArrayCL ba3(96);
        for (int i = 0; i < 96; i++) {
            if (i % 2 == 0) {
                ba3.set_bit(i);
            }
        }
        for (int i = 0; i < 96; i++) {
            if (ba3.get_bit(i) != ((i + 1) % 2)) {
                outcome = false;
            }
        }
        test_result(outcome, "Constructor non-zero");
    }

    void run_test_get_bit_out_of_bounds () {
        BitArrayCL ba(96);
        bool outcome = true;
        if (ba.get_bit(-1) != 0) {
            outcome = false;
        }
        if (ba.get_bit(96) != 0) {
            outcome = false;
        }
        if (ba.get_bit(100) != 0) {
            outcome = false;
        }
        test_result(outcome, "Get bit out of bounds");
    }

    void run_test_clear_bit_out_of_bounds () {
        bool outcome = true;
        try {
            BitArrayCL ba(96);
            ba.clear_bit(96);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds");

        try {
            BitArrayCL ba(96);
            ba.clear_bit(-1);
        } catch (const std::out_of_range &e) {
            outcome = false;
        }
        test_result(outcome, "Clear bit out of bounds negative");
    }

    void run_test_toggle_behavior () {
        bool outcome = true;
        BitArrayCL ba(96);
        int test_positions[] = {0, 31, 32, 63, 64, 95};
        for (int idx = 0; idx < 6; idx++) {
            int i = test_positions[idx];
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
            ba.clear_bit(i);
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
            ba.set_bit(i);
            if (ba.get_bit(i) != 1) {
                outcome = false;
            }
        }
        test_result(outcome, "Toggle behavior");
        verify_serialize_deserialize(ba, "Toggle behavior");
    }

    void run_test_cross_array_independence () {
        BitArrayCL ba(96);
        ba.set_bit(0);
        ba.set_bit(32);
        ba.set_bit(64);
        bool outcome = true;
        if (ba.get_bit(0) != 1) {
            outcome = false;
        }
        if (ba.get_bit(32) != 1) {
            outcome = false;
        }
        if (ba.get_bit(64) != 1) {
            outcome = false;
        }
        for (int i = 1; i < 32; i++) {
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        for (int i = 33; i < 64; i++) {
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        for (int i = 65; i < 96; i++) {
            if (ba.get_bit(i) != 0) {
                outcome = false;
            }
        }
        test_result(outcome, "Cross array independence");
        verify_serialize_deserialize(ba, "Cross array independence");
    }

    void run_test_mid_array_positions () {
        int test_positions[] = {15, 47, 79};
        bool outcome = true;
        for (int idx = 0; idx < 3; idx++) {
            int bit_idx = test_positions[idx];
            BitArrayCL ba(96);
            ba.set_bit(bit_idx);
            int count = 0;
            for (int i = 0; i < 96; i++) {
                count += ba.get_bit(i);
            }
            if (count != 1) {
                outcome = false;
            }
            if (ba.get_bit(bit_idx) != 1) {
                outcome = false;
            }
            verify_serialize_deserialize(ba, "Mid array positions");
        }
        test_result(outcome, "Mid array positions");
    }

    void run_test_full_range_coverage () {
        BitArrayCL ba(96);
        ba.set_bit(0);
        ba.set_bit(32);
        ba.set_bit(64);
        bool outcome = true;
        int count = 0;
        for (int i = 0; i < 96; i++) {
            count += ba.get_bit(i);
        }
        if (count != 3) {
            outcome = false;
        }
        if (ba.get_bit(0) != 1 || ba.get_bit(32) != 1 || ba.get_bit(64) != 1) {
            outcome = false;
        }
        test_result(outcome, "Full range coverage");
        verify_serialize_deserialize(ba, "Full range coverage");
    }

    void run_test_multiple_arrays_simultaneously () {
        BitArrayCL ba(96);
        for (int i = 0; i < 96; i += 3) {
            ba.set_bit(i);
        }
        bool outcome = true;
        int count = 0;
        for (int i = 0; i < 96; i++) {
            if (i % 3 == 0) {
                if (ba.get_bit(i) != 1) {
                    outcome = false;
                }
                count++;
            } else {
                if (ba.get_bit(i) != 0) {
                    outcome = false;
                }
            }
        }
        if (count != 32) {
            outcome = false;
        }
        test_result(outcome, "Multiple arrays simultaneously");
        verify_serialize_deserialize(ba, "Multiple arrays simultaneously");
    }

    void print_test_result () {
        printf("*** SUMMARY: %d tests, %d passed\n", test_count, test_success);
    }

private:
    void test_result (bool outcome, const char *test_name, int value) {
        std::string test_name_str = std::string(test_name) + std::to_string(value);
        test_result(outcome, test_name_str.c_str());
    }

    void test_result (bool outcome, const char *test_name) {
        if (outcome) {
            test_success++;
            if (verbose) {
                printf(TEST_PASSED "%s\n", test_name);
            }
        } else {
            printf(TEST_FILED "%s\n", test_name);
        }
        test_count++;
    }

    void verify_serialize_deserialize (const BitArrayCL& ba, const char* test_name) {
        std::stringstream ss;
        ba.serialize(ss);
        BitArrayCL* ba_deserialized = BitArrayCL::deserialize(ss);
        bool outcome = true;
        for (int i = 0; i < 96; i++) {
            if (ba.get_bit(i) != ba_deserialized->get_bit(i)) {
                outcome = false;
            }
        }
        delete ba_deserialized;
        std::string ser_test_name = std::string(test_name) + " serialize/deserialize";
        test_result(outcome, ser_test_name.c_str());
    }

    int test_count = 0;
    int test_success = 0;
    bool verbose = true;
};

//================================================================================================================================
//=> - Test suites -
//================================================================================================================================

void test_suite_BitArray32 () {
    Tester_BitArray32 tester;
    tester.test_zero_initialization();
    tester.test_zero_initialization_explicit();
    tester.run_test_no_crash_limits();
    tester.run_test_bit_masks_validation();
    tester.run_test_alternate_bits();
    tester.run_test_bit_mapping();
    tester.run_test_single_bit_set();
    tester.run_test_clear_bit();
    tester.run_test_clear_bit_already_clear();
    tester.run_test_constructor_non_zero();
    tester.run_test_get_bit_out_of_bounds();
    tester.run_test_clear_bit_out_of_bounds();
    tester.run_test_toggle_behavior();
    tester.print_test_result(); 
}

void test_suite_BitArrayCL32 () {
    Tester_BitArrayCL32 tester;
    tester.test_zero_initialization();
    tester.run_test_no_crash_limits();
    tester.run_test_alternate_bits();
    tester.run_test_bit_mapping();
    tester.run_test_single_bit_set();
    tester.run_test_clear_bit();
    tester.run_test_clear_bit_already_clear();
    tester.run_test_constructor_non_zero();
    tester.run_test_get_bit_out_of_bounds();
    tester.run_test_clear_bit_out_of_bounds();
    tester.run_test_toggle_behavior();
    tester.print_test_result();
}

void test_suite_BitArrayCL96 () {
    Tester_BitArrayCL96 tester;
    tester.test_zero_initialization();
    tester.run_test_no_crash_limits();
    tester.run_test_alternate_bits();
    tester.run_test_single_bit_set();
    tester.run_test_clear_bit();
    tester.run_test_clear_bit_already_clear();
    tester.run_test_constructor_non_zero();
    tester.run_test_get_bit_out_of_bounds();
    tester.run_test_clear_bit_out_of_bounds();
    tester.run_test_toggle_behavior();
    tester.run_test_cross_array_independence();
    tester.run_test_mid_array_positions();
    tester.run_test_full_range_coverage();
    tester.run_test_multiple_arrays_simultaneously();
    tester.print_test_result();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    
    //test_suite_BitArray32();
    //test_suite_BitArrayCL32();
    test_suite_BitArrayCL96();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
