// Copyright Thiago Alves, May 2016
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissionsand
// limitations under the License.

#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "md5.h"

using namespace std;

/**
 * \defgroup glue_generator Glue Generator
 * @brief Glue generator produces a binding headers for connecting the MATIEC
 * Structured Text to C application code to the OpenPLC runtime.
 *
 */

/** \addtogroup glue_generator
 *  @{
 */

/// Defines the information we need about a particular variable
/// in order to generate the glue structures.
struct IecVar {
    string name;
    string type;
    uint16_t pos1;
    uint16_t pos2;
};

/// Write the header to the output stream. The header is common among all
/// glueVars files.
/// @param glueVars The output stream to write to.
void generateHeader(ostream& glueVars) {
    glueVars << R"(// This file is responsible for gluing the variables from the IEC program to
// the OpenPLC memory pointers. It is automatically generated by the
// glue_generator program. PLEASE DON'T EDIT THIS FILE!
// Thiago Alves, May 2016
// -----------------------------------------------------------------------------
#include <cstdint>

#include "iec_std_lib.h"

TIME __CURRENT_TIME;
extern unsigned long long common_ticktime__;

#ifndef OPLC_IEC_GLUE_DIRECTION
#define OPLC_IEC_GLUE_DIRECTION
enum IecLocationDirection {
    IECLDT_IN,
    IECLDT_OUT,
    IECLDT_MEM,
};
#endif  // OPLC_IEC_GLUE_DIRECTION

#ifndef OPLC_IEC_GLUE_SIZE
#define OPLC_IEC_GLUE_SIZE
enum IecLocationSize {
    /// Variables that are a single bit
    IECLST_BIT,
    /// Variables that are 1 byte
    IECLST_BYTE,
    /// Variables that are 2 bytes
    IECLST_WORD,
    /// Variables that are 4 bytes, including REAL
    IECLST_DOUBLEWORD,
    /// Variables that are 8 bytes, including LREAL
    IECLST_LONGWORD,
};
#endif  // OPLC_IEC_GLUE_SIZE

#ifndef OPLC_IEC_GLUE_VALUE_TYPE
#define OPLC_IEC_GLUE_VALUE_TYPE
enum IecGlueValueType {
    IECVT_BOOL,
    IECVT_BYTE,
    IECVT_SINT,
    IECVT_USINT,
    IECVT_INT,
    IECVT_UINT,
    IECVT_WORD,
    IECVT_DINT,
    IECVT_UDINT,
    IECVT_DWORD,
    IECVT_REAL,
    IECVT_LREAL,
    IECVT_LWORD,
    IECVT_LINT,
    IECVT_ULINT,
    /// This is not a normal type and won't appear in the glue variables
    /// here. But it does allow you to create your own indexed mapping
    /// and have a way to indicate a value that is not assigned a type.
    IECVT_UNASSIGNED,
};
#endif  // OPLC_IEC_GLUE_VALUE_TYPE

#ifndef OPLC_GLUE_BOOL_GROUP
#define OPLC_GLUE_BOOL_GROUP
/// Defines the mapping for a glued variable that is a boolean array.
/// The boolean array is sub-indiced as a group, for example all of the
/// values %IX0.0 through %IX0.1 share the same group. The size of the
/// group is fixed at 8 values, but some may be unassigned.
struct GlueBoolGroup {
    /// The first index for this array. If we are iterating over the glue
    /// variables, then this index is superfluous, but it is very helpful
    /// for debugging.
    std::uint16_t index;
    /// The values in this group. If the value is not assigned, then the
    /// value at the index points to nullptr.
    IEC_BOOL* values[8];
};
#endif // OPLC_GLUE_BOOL_GROUP

#ifndef OPLC_GLUE_VARIABLE
#define OPLC_GLUE_VARIABLE
/// Defines the mapping for a glued variable. This defines a simple, space
/// efficient lookup table. It has all of the mapping information that you
/// need to find the variable based on the location name (e.g. %IB1.1). While
/// this is space efficient, this should be searched once to construct a fast
/// lookup into this table used for the remainder of the application lifecycle.
struct GlueVariable {
    /// The direction of the variable - this is determined by I/Q/M.
    IecLocationDirection dir;
    /// The size of the variable - this is determined by X/B/W/D/L.
    IecLocationSize size;
    /// The most significant index for the variable. This is the part of the
    /// name, converted to an integer, before the period.
    std::uint16_t msi;
    /// The least significant index (sub-index) for the variable. This is the
    /// part of the name, converted to an integer, after the period. It is
    /// only relevant for boolean (bit) values.
    std::uint8_t lsi;
    /// The type of the glue variable. This is used so that we correctly
    /// write into the value type.
    IecGlueValueType type;
    /// A pointer to the memory address for reading/writing the value.
    void* value;
};
#endif  // OPLC_GLUE_VARIABLE

// Internal buffers for I/O and memory. These buffers are defined in the
// auto-generated glueVars.cpp file.
// Inputs: I
// Outputs: Q
// Memory: M
#define BUFFER_SIZE 1024

// Booleans - defined by the "X" width
IEC_BOOL *bool_input[BUFFER_SIZE][8] = {};
IEC_BOOL *bool_output[BUFFER_SIZE][8] = {};

// Bytes - defined by the "B" width
IEC_BYTE *byte_input[BUFFER_SIZE] = {};
IEC_BYTE *byte_output[BUFFER_SIZE] = {};

// Words - defined by the "W" width
IEC_UINT *int_input[BUFFER_SIZE] = {};
IEC_UINT *int_output[BUFFER_SIZE] = {};
IEC_UINT *int_memory[BUFFER_SIZE] = {};

// Double words - defined by the "D" width
// This is also valid size for a REAL but we don't allow
// them in this structure
IEC_DINT *dint_memory[BUFFER_SIZE] = {};

// Longs - defined by the "L" width
// This is also valid size for a LREAL but we don't allow
// them in this structure
IEC_LINT *lint_memory[BUFFER_SIZE] = {};

// Special Functions
IEC_LINT *special_functions[BUFFER_SIZE];


#define __LOCATED_VAR(type, name, ...) type __##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR
#define __LOCATED_VAR(type, name, ...) type* name = &__##name;
#include "LOCATED_VARIABLES.h"
#undef __LOCATED_VAR
)";
}

int parseIecVars(istream& locatedVars, char *varName, char *varType, md5_state_s& md5_state) {
    string line;
    char buffer[1024];

    if (getline(locatedVars, line)) {
        md5_append(&md5_state, reinterpret_cast<const md5_byte_t*>(line.c_str()), line.length());
        int i = 0, j = 0;
        strncpy(buffer, line.c_str(), 1024);
        for (i = 0; buffer[i] != '('; i++) {
            continue;
        }
        i++;

        while (buffer[i] != ',') {
            varType[j] = buffer[i];
            i++; j++;
            varType[j] = '\0';
        }
        i++;
        j = 0;

        while (buffer[i] != ',') {
            varName[j] = buffer[i];
            i++; j++;
            varName[j] = '\0';
        }

        return 1;
    } else {
        return 0;
    }
}

void findPositions(const char *varName, uint16_t *pos1, uint16_t *pos2) {
    int i = 4, j = 0;
    char tempBuffer[100];

    while (varName[i] != '_' && varName[i] != '\0') {
        tempBuffer[j] = varName[i];
        i++; j++;
        tempBuffer[j] = '\0';
    }
    *pos1 = atoi(tempBuffer);

    if (varName[i] == '\0') {
        *pos2 = 0;
        return;
    }

    j = 0; i++;

    while (varName[i] != '\0') {
        tempBuffer[j] = varName[i];
        i++; j++;
        tempBuffer[j] = '\0';
    }
    *pos2 = atoi(tempBuffer);
}

void glueVar(ostream& glueVars, const char *varName, uint16_t pos1,
             uint16_t pos2) {
    if (pos2 >= 8) {
        cout << "***Invalid addressing on located variable" << varName;
        cout << "***" << endl;
    }

    if (varName[2] == 'I') {
        // INPUT
        switch (varName[3]) {
            case 'X':
                glueVars << "\tbool_input[" << pos1 << "][" << pos2 << "] = " \
                         << varName << ";\n";
                break;
            case 'B':
                glueVars << "\tbyte_input[" << pos1 << "] = " \
                         << varName << ";\n";
                break;
            case 'W':
                glueVars << "\tint_input[" << pos1 << "] = " \
                         << varName << ";\n";
                break;
        }
    } else if (varName[2] == 'Q') {
        // OUTPUT
        switch (varName[3]) {
            case 'X':
                glueVars << "\tbool_output[" << pos1 << "][" << pos2 << "] = " \
                         << varName << ";\n";
                break;
               case 'B':
                glueVars << "\tbyte_output[" << pos1 << "] = "
                         << varName << ";\n";
                break;
            case 'W':
                glueVars << "\tint_output[" << pos1 << "] = " \
                         << varName << ";\n";
                break;
        }
    } else if (varName[2] == 'M') {
        // MEMORY
        switch (varName[3]) {
            case 'W':
                glueVars << "\tint_memory[" << pos1 << "] = " \
                         << varName << ";\n";
                break;
            case 'D':
                glueVars << "\tdint_memory[" << pos1 << "] = (IEC_DINT *)" \
                         << varName << ";\n";
                break;
            case 'L':
                if (pos1 > 1023)
                    glueVars << "\tspecial_functions[" << (pos1-1024) \
                             << "] = (IEC_LINT *)" << varName << ";\n";
                else
                    glueVars << "\tlint_memory[" << pos1 << "] = (IEC_LINT *)" \
                             << varName << ";\n";
                break;
        }
    }
}

const char* fromDirectionFlag(const char flag) {
    switch (flag) {
        case 'I':
            return "IN";
        case 'Q':
            return "OUT";
        default:
            return "MEM";
    }
}

const char* fromSizeFlag(const char flag) {
    switch (flag) {
        // G is a special flag we use to represent a boolean group
        case 'G':
        case 'X':
            return "BIT";
        case 'B':
            return "BYTE";
        case 'W':
            return "WORD";
        case 'D':
            return "DOUBLEWORD";
        case 'L':
        default:
            return "LONGWORD";
    }
}

void generateBoolGroups(ostream& glueVars, char direction, map<uint16_t, array<string, 8>>& items) {
    if (items.empty()) {
        return;
    }

    for (auto it_group = items.begin(); it_group != items.end(); ++it_group) {
        string name;
        name += direction;
        name += 'G';
        name += to_string(it_group->first);
        glueVars << "GlueBoolGroup ___" << name;
        glueVars << " { .index=" << it_group->first << ", .values={ ";
        for (auto it_var = it_group->second.begin(); it_var != it_group->second.end(); ++it_var) {
            if (it_var->empty()) {
                glueVars << "nullptr, ";
            } else {
                glueVars << (*it_var) << ", ";
            }
        }
        glueVars <<  "} };\n";
        glueVars << "GlueBoolGroup* __" << name << "(&___" << name << ");\n";
    }
}

/// Generate the boolean groups structures. These structures contain
/// boolean values that are grouped together. For example, %IX0.0 and %IX0.1
/// are generated as a group and then that group is referred to from
/// the integrated glue. This function generates the definitions
/// of the bool groups.
void generateBoolGroups(ostream& glueVars, list<IecVar>& all_vars) {
    map<uint16_t, array<string, 8>> inputs;
    map<uint16_t, array<string, 8>> outputs;
    map<uint16_t, array<string, 8>> memory;
    for (auto it_var = all_vars.begin(); it_var != all_vars.end(); ) {
        const char sizeFlag = it_var->name[3];
        if (sizeFlag != 'X') {
            // We only care about the boolen locations here.
            ++it_var;
            continue;
        }
        const char directionFlag = it_var->name[2];

        map<uint16_t, array<string, 8>>* container;
        switch (directionFlag) {
            case 'I':
                container = &inputs;
                break;
            case 'Q':
                container = &outputs;
                break;
            default:
                container = &memory;
                break;
        }

        uint16_t pos1 = it_var->pos1;
        uint16_t pos2 = it_var->pos2;
        string name = it_var->name;

        auto it_group = container->find(pos1);
        if (it_group == container->end()) {
            (*container)[pos1] = array<string, 8>();
            // Replace the name with our special name
            it_var->name = "__";
            it_var->name += directionFlag;
            it_var->name += 'G';
            it_var->name += to_string(pos1);
            it_var->pos2 = 0;
            ++it_var;
        } else {
            // Since we have seen this before, remove it from the
            // list of variables so that when we generate the
            // integrated glue, we only see the first item in
            // the group, not each item
            it_var = all_vars.erase(it_var);
        }

        (*container)[pos1][pos2] = name;
    }

    // Now that we have them grouped into the groups that we care about
    // generate the intermediate glue bindings for the items
    generateBoolGroups(glueVars, 'I', inputs);
    generateBoolGroups(glueVars, 'Q', outputs);
    generateBoolGroups(glueVars, 'M', memory);
}

void generateIntegratedGlue(ostream& glueVars, const list<IecVar>& all_vars) {
    glueVars << "/// The size of the array of glue variables.\n";
    glueVars << "extern std::size_t const OPLCGLUE_GLUE_SIZE(";
    glueVars << all_vars.size() << ");\n";

    glueVars << "/// The packed glue variables.\n";
    glueVars << "extern const GlueVariable oplc_glue_vars[] = {\n";
    for (auto it = all_vars.begin(); it != all_vars.end(); ++it) {
        string name = it->name;
        const char directionFlag = it->name[2];
        const char sizeFlag = it->name[3];

        glueVars << "    {";
        glueVars << " IECLDT_" << fromDirectionFlag(directionFlag) << ",";
        glueVars << " IECLST_" << fromSizeFlag(sizeFlag) << ",";
        glueVars << " " << it->pos1 << ",";
        glueVars << " " << it->pos2 << ",";
        glueVars << " IECVT_" << it->type << ", ";
        glueVars << " " << name << " },\n";
    }
    glueVars << "};\n\n";
}

void generateBottom(ostream& glueVars) {
    glueVars << R"(void updateTime()
{
    __CURRENT_TIME.tv_nsec += common_ticktime__;

    if (__CURRENT_TIME.tv_nsec >= 1000000000) {
        __CURRENT_TIME.tv_nsec -= 1000000000;
        __CURRENT_TIME.tv_sec += 1;
    }
})";
}

void generateBody(istream& locatedVars, ostream& glueVars, md5_byte_t digest[16]) {
    // We will generate a checksum of the located variables so that
    // we can detect likely changes (not intended to be cryptographically
    // secure). This is only to prevent obvious problems.
    md5_state_s md5_state;
    md5_init(&md5_state);

    // Start the generation process. We need to know all of the variables
    // in advance so that we can appropriately size some storage based
    // on the variables that are actually needed.

    list<IecVar> all_vars;

    // Keep track of the counts of types that we care about

    char iecVar_name[100];
    char iecVar_type[100];
    int32_t max_index(-1);

    while (parseIecVars(locatedVars, iecVar_name, iecVar_type, md5_state)) {
        cout << "varName: " << iecVar_name << "\tvarType: " << iecVar_type;
        cout << endl;

        // Get the indices these reference. The second position is
        // only relevant for boolean types that pack 8 bits into one
        // value.
        uint16_t pos1;
        uint16_t pos2;
        findPositions(iecVar_name, &pos1, &pos2);
        max_index = max(max_index, (int32_t)pos1);

        all_vars.push_back(IecVar{ iecVar_name, iecVar_type, pos1, pos2 });
    }

    // Generate the classical glue variables
    glueVars << "void glueVars()\n{\n";
    for (auto it = all_vars.begin(); it != all_vars.end(); ++it) {
        glueVar(glueVars, (*it).name.c_str(), (*it).pos1, (*it).pos2);
    }
    glueVars << "}\n\n";

    // Generate the unified glue variables
    generateBoolGroups(glueVars, all_vars);
    generateIntegratedGlue(glueVars, all_vars);

    // Finish the checksum value
    md5_finish(&md5_state, digest);
}

void generateChecksum(ostream& glueVars, md5_byte_t digest[16]) {
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    // We do this in a separate step, otherwise every unit test would have
    // this value.
    glueVars << "/// MD5 checksum of the located variables.\n";
    glueVars << "/// WARNING: this must not be used to trust file contents.\n";
    glueVars << "extern const char OPLCGLUE_MD5_DIGEST[] = {";
    for (int i = 0; i <= 16; ++i) {
       glueVars << '\'' << hex_chars[ ( digest[i] & 0xF0 ) >> 4 ] << "\', ";
       glueVars << '\'' << hex_chars[ ( digest[i] & 0x0F ) >> 0 ] << "\', ";
    }
    glueVars << "};\n";
    glueVars << "\n\n";
}

/// This is our main function. We define it with a different name and then
/// call it from the main function so that we can mock it for the purpose
/// of testing.
int mainImpl(int argc, char const * const *argv) {
    // Parse the command line arguments - if they exist. Show the help if there
    // are too many arguments or if the first argument is for help.
    bool show_help = argc >= 2 \
        && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0);
    if (show_help || (argc != 1 && argc != 3)) {
        cout << "Usage " << endl << endl;
        cout << "  glue_generator [options] <path-to-located-variables.h> " \
                "<path-to-glue-vars.cpp>" << endl << endl;
        cout << "Reads the LOCATED_VARIABLES.h file generated by the MATIEC " \
                "compiler and produces" << endl;
        cout << "glueVars.cpp for the OpenPLC runtime. If not specified, " \
                "paths are relative to" << endl;
        cout << "the current directory." << endl << endl;
        cout << "Options" << endl;
        cout << "  --help,-h   = Print usage information and exit." << endl;
        // Give a non-zero return code if the usage was incorrect
        return show_help ? 0 : -1;
    }

    // If we have 3 arguments, then the user provided input and output paths
    string input_file_name("LOCATED_VARIABLES.h");
    string output_file_name("glueVars.cpp");
    if (argc == 3) {
        input_file_name = argv[1];
        output_file_name = argv[2];
    }

    // Try to open the files for reading and writing.
    ifstream locatedVars(input_file_name, ios::in);
    if (!locatedVars.is_open()) {
        cout << "Error opening located variables file at " << input_file_name;
        cout << endl;
        return 1;
    }
    ofstream glueVars(output_file_name, ios::trunc);
    if (!glueVars.is_open()) {
        cout << "Error opening glue variables file at " << output_file_name;
        cout << endl;
        return 2;
    }

    generateHeader(glueVars);

    md5_byte_t digest[16];
    generateBody(locatedVars, glueVars, digest);
    generateChecksum(glueVars, digest);
    generateBottom(glueVars);

    return 0;
}

// For testing, we need to allow omitting the main function defined here.
#ifndef OPLCGLUE_OMIT_MAIN
int main(int argc, char *argv[]) {
    return mainImpl(argc, argv);
}
#endif  // OPLCGLUE_OMIT_MAIN

/** @}*/
