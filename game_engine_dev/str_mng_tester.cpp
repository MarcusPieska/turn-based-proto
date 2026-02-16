//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>
#include <stdexcept>
#include <sstream>

#include "str_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

using std::string;
using std::vector;

int test_count = 0;
int test_pass = 0;
bool verbose = true;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

void note_test_result (bool cond, cstr msg) {
    test_count++;
    if (cond) {
        test_pass++;
        if (verbose) {
            printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        printf("*** TEST FAILED: %s\n", msg);
    }
}

void note_test_result (bool cond, cstr msg1, cstr msg2) {
    std::string msg = std::string(msg1) + std::string(msg2);
    note_test_result (cond, msg.c_str());
}

void confirm_split_result (const vector<string>& result, const vector<string>& expected, cstr msg) {
    if (result.size() != expected.size()) {
        note_test_result (false, msg, ": size mismatch");
        return;
    }
    for (size_t i = 0; i < result.size(); i++) {
        string result_str = result[i];
        string expected_str = expected.at(i);
        if (result_str != expected_str) {
            note_test_result (false, msg, ": element mismatch");
            return;
        }
    }
    note_test_result (true, msg);
}

void confirm_read_result (const string& result, const string& expected, cstr msg) {
    if (result != expected) {
        note_test_result (false, msg, ": content mismatch");
        return;
    }
    note_test_result (true, msg);
}

void summarize_test_results () {
    printf("--------------------------------\n");
    printf(" Test count: %d\n", test_count);
    printf(" Test pass: %d\n", test_pass);
    printf(" Test fail: %d\n", test_count - test_pass);
    printf("--------------------------------\n\n\n");

    test_count = 0;
    test_pass = 0;
}

//================================================================================================================================
//=> - Test functions -
//================================================================================================================================

void test_string_splitter () {
    string str;
    string delimiter;
    vector<string> result;
    vector<string> expected;

    str = "Hello,World,This,Is,A,Test";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "Simple string split");

    str = "Hello - World - This - Is - A - Test";
    delimiter = " - ";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "Simple string split, longer delimiter");

    str = "Hello - World - This - Is - A - Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello ", " World ", " This ", " Is ", " A ", " Test"};
    confirm_split_result (result, expected, "Simple string split, delimiter with remaining spaces");

    str = "Hello - World - This - Is - A - Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = StringTrimmer(" ").trim(result[i]);
    }
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "Simple string split, short delimiter, with trimming");

    str = "Hello - World - This - *|Is* - A - Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = StringTrimmer(" |*").trim(result[i]);
    }
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "Simple string split, short delimiter, with multiple-char trimming");

    str = "";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {""};
    confirm_split_result (result, expected, "Empty string input");

    str = "Hello,World";
    delimiter = "";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello,World"};
    confirm_split_result (result, expected, "Empty delimiter");

    str = "HelloWorld";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"HelloWorld"};
    confirm_split_result (result, expected, "String with no delimiter");

    str = ",Hello,World";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"", "Hello", "World"};
    confirm_split_result (result, expected, "String starting with delimiter");

    str = "Hello,World,";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World", ""};
    confirm_split_result (result, expected, "String ending with delimiter");

    str = "Hello,,World";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "", "World"};
    confirm_split_result (result, expected, "String with consecutive delimiters");

    str = ",";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"", ""};
    confirm_split_result (result, expected, "String is just delimiter");

    str = ",,";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"", "", ""};
    confirm_split_result (result, expected, "String is consecutive delimiters");

    str = "A";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"A"};
    confirm_split_result (result, expected, "Single character string");

    str = "Hello---World";
    delimiter = "---";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World"};
    confirm_split_result (result, expected, "Multi-character delimiter, no matches in middle");

    summarize_test_results();
}

void test_string_splitter_failures () {
    string str;
    string delimiter;
    vector<string> result;
    vector<string> expected;

    str = "Hello,World,This,Is,A-Test";
    delimiter = ",";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "FAILURE: Simple string split");

    str = "Hello - World - This - Is - A -  Test";
    delimiter = " - ";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "FAILURE: Simple string split, longer delimiter");

    str = "Hello - World - This - Is - A -Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    expected = {"Hello ", " World ", " This ", " Is ", " A ", " Test"};
    confirm_split_result (result, expected, "FAILURE: Simple string split, delimiter with remaining spaces");

    str = "Hello - World - This - Is - A - *Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = StringTrimmer(" ").trim(result[i]);
    }
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "FAILURE: Simple string split, short delimiter, with trimming");

    str = "Hello - World - This - *|Isz - A - Test";
    delimiter = "-";
    result = StringSplitter(delimiter).split(str);
    for (size_t i = 0; i < result.size(); i++) {
        result[i] = StringTrimmer(" |*").trim(result[i]);
    }
    expected = {"Hello", "World", "This", "Is", "A", "Test"};
    confirm_split_result (result, expected, "FAILURE: Simple string split, short delimiter, with multiple-char trimming");

    summarize_test_results();
}

void test_string_io_operations () {
    string filename;
    string content;
    string result;
    
    filename = "test.temp";
    content = "Hello,World,This,Is,A,Test";
    StringWriter writer = StringWriter(filename);
    writer.append(content);
    writer.write();
    StringReader reader = StringReader(filename);
    result = reader.read();
    confirm_read_result (result, content, "Simple string read");
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    //test_string_splitter();
    //test_string_splitter_failures();
    test_string_io_operations();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
