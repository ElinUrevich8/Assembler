# Clean and build the assembler
echo "Cleaning and building the assembler..."
make clean
make 
if [ $? -ne 0 ]; then
    echo "Error: Make failed. Please check the compilation."
    exit 1
fi
echo "----------------------------------------"

# Tests of stage 1: the preassembler functionality
# --------------Expected success-------------------
# 1. Regular macro definition
# 2. Multiple Macros: Test multiple non-nested macros in one file.
# 3. Empty Macro: Test a macro with no content.

# --------------Expected error-------------------
# 4. Invalid Macro Name: Test reserved words
# 5. Invalid Macro Name: Invalid characters in the macro name (in macrodef).
# 6. Invalid Macro Name: Invalid characters in the macro name (in macroend).
# 7. Long Lines: Test lines exceeding 80 characters.
# 8. Invalid Macro Name: Names exceeding 30 characters.
# 9. Duplicate Macros: Test redefining a macro with the same name. 

# --------------Assumptions-------------------
# Unclosed Macro: Test a macro definition missing macroend.
# Empty File: Test an empty input file or one with only whitespace.



# Array of test files and descriptions
tests=(
    "makro1.as:Regular macro definition"
    "makro2.as:Multiple macro definition"
    "makro3.as:Empty macros"
    "makro4.as:Invalid macro name (reserved words)"
    "makro5.as:Invalid macro name (invalid characters in start definition)"
    "makro6.as:Invalid macro name (invalid characters in end definition)"
    "makro7.as:Long lines exceeding 80 characters"
    "makro8.as:Invalid macro name (exceeding 30 characters)"
    "makro9.as:Duplicate macro definition"
)

# Expected outcomes (0 for error, 1 for success)
expected_status=(
    1  # makro1.as
    1  # makro3.as
    1  # makro4.as
    0  # makro5.as
    0  # makro6.as
    0  # makro7.as
    0  # makro8.as
    0  # makro9.as
)

# Counter for test number
test_num=0

# Loop through tests
while [ $test_num -lt ${#tests[@]} ]; do
    # Extract filename and description
    filename=$(echo "${tests[$test_num]}" | cut -d':' -f1)
    description=$(echo "${tests[$test_num]}" | cut -d':' -f2)
    expected=${expected_status[$test_num]}

    # Increment test number for display (1-based)
    ((test_num_display=test_num+1))

    echo "--------------Test $test_num_display: $description--------------------"
    echo "Running assembler on filesamples/$filename"
    echo -e "Input file contents:"
    cat "filesamples/$filename"
    echo -e "\nExpected output:"
    if [ $expected -eq 1 ]; then
        echo "The assembler should generate filesamples/${filename%.as}.am with expanded macros"
    else
        echo "The assembler should fail and remove filesamples/${filename%.as}.am"
    fi
    echo "Running assembler..."
    ./assembler "filesamples/$filename" 2> "debug_${filename%.as}.log"
    echo -e "\nDebug output:"
    cat "debug_${filename%.as}.log"
    echo -e "\nOutput file contents (if exists):"
    if [ -f "filesamples/${filename%.as}.am" ]; then
        cat -e "filesamples/${filename%.as}.am"
    else
        echo "No output file generated (filesamples/${filename%.as}.am does not exist)"
    fi
    echo "----------------------------------------"

    ((test_num++))
done

