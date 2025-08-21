import unittest
from pathlib import Path
import subprocess # Added for subprocess.run in compile_assembler and execute_assembler

THIS_FILE = Path(__file__).resolve()
THIS_DIRECTORY = THIS_FILE.parent
assert THIS_DIRECTORY.name == "tests", "This script must be run from the 'tests' directory."
TESTS = THIS_DIRECTORY
CASES = TESTS / "cases" # Base directory for all test cases


def compile_assembler():
    """
    Compiles the assembler source code.
    This function assumes that the assembler source code is in the parent directory
    and uses a Makefile.
    """
    SOURCE_CODE_DIRECTORY = THIS_DIRECTORY.parent
    assert (SOURCE_CODE_DIRECTORY / "Makefile").exists(), "Assembler source code directory must contain a Makefile."

    try:
        # Clean previous builds
        subprocess.run(
            ["make", "clean"],
            check=True,
            cwd=str(SOURCE_CODE_DIRECTORY),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # Build the assembler
        subprocess.run(
            ["make"],
            check=True,
            cwd=str(SOURCE_CODE_DIRECTORY),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as e:
        stdout = e.stdout.decode() if e.stdout else ""
        stderr = e.stderr.decode() if e.stderr else ""
        raise RuntimeError(
            "Compilation failed.\n\n=== STDOUT ===\n{}\n\n=== STDERR ===\n{}\n".format(stdout, stderr)
        )


def execute_assembler(input_file, allow_failure=False):
    """
    Executes the assembler on the given input file.
    Requirement: Run assembler WITHOUT the '.as' extension (strip it before invoking).
    If allow_failure is True, do not raise on non-zero exit code.
    Returns (returncode, stdout, stderr).
    """
    # Strip `.as` before calling assembler, e.g. ".../test.as" -> ".../test"
    input_without_ext = input_file.with_suffix('')

    try:
        result = subprocess.run(
            ["./assembler", str(input_without_ext)], # Run without .as extension
            check=not allow_failure, # Only check if failure is not allowed
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        # If failure is allowed, return the captured output; else re-raise
        if allow_failure:
            return e.returncode, e.stdout, e.stderr
        else:
            raise


def discover_test_cases():
    """
    Discover test case directories under CASES/.
    A test directory is any directory that contains a 'test.as' file.
    The presence of an 'expected.am' file indicates expected success.
    The presence of an 'expected.error' or 'expected.err' file indicates expected failure.
    """
    for test_dir in sorted(CASES.rglob("*")):
        if not test_dir.is_dir():
            continue
        if (test_dir / "test.as").exists():
            # Detect if test is an expected-failure case
            is_expected_failure = (test_dir / "expected.error").exists() or \
                                  (test_dir / "expected.err").exists()
            test_name = str(test_dir.relative_to(CASES)) # Get a relative path for the test name
            yield test_name, test_dir, is_expected_failure


def make_test_function(test_name, test_directory, is_expected_failure):
    """
    Create and return a test function for the given test case.
    The function will:
    - Compile the assembler before running tests (done once in __main__)
    - Run the assembler on 'test.as'
    - If success expected: compare produced .am (or pass2 outputs) against expected.am
    - If failure expected: compare stderr against expected.error / expected.err
    """
    def test(self):
        input_file_path = test_directory / "test.as"

        def cleanup_test():
            """
            Cleanup any generated files: .am, .ob, .ent, .ext, and any intermediate files.
            """
            # Remove files with known extensions
            generated_extensions = ['.am', '.ob', '.ent', '.ext']
            for ext in generated_extensions:
                generated_file = input_file_path.with_suffix(ext)
                if generated_file.exists():
                    generated_file.unlink()

        def _compare_multi_extension_outputs(input_file_path, test_directory, test_name):
            """
            Compare the content of .ob, .ent, and .ext files for pass2 tests.
            Only compare files that have a corresponding expected.* file present.
            """
            extensions = ['.ob', '.ent', '.ext']
            for ext in extensions:
                expected_file = test_directory / ("expected" + ext)
                actual_file = input_file_path.with_suffix(ext)

                # Only compare if expected file exists
                if expected_file.exists():
                    # Fail with explicit error if actual file does not exist
                    if not actual_file.exists():
                        raise FileNotFoundError(
                            "Test {}: Actual output file {} does not exist.".format(test_name, actual_file)
                        )

                    # Compare file content
                    actual_content = actual_file.read_text().strip()
                    expected_content = expected_file.read_text().strip()

                    assert actual_content == expected_content, \
                        "Test {}: Output file '{}' content does not match expected.\n" \
                        "--- Got ---\n{}\n\n--- Expected ---\n{}".format(
                            test_name, actual_file.name, actual_content, expected_content)

        try:
            # Validate input file existence and type
            if not input_file_path.exists():
                raise FileNotFoundError("Input file {} does not exist.".format(input_file_path))
            if not input_file_path.is_file():
                raise ValueError("Input path {} is not a file.".format(input_file_path))
            if not input_file_path.suffix == '.as':
                raise ValueError("Input file {} must have a .as extension.".format(input_file_path))

            if is_expected_failure:
                # If an error is expected, run the assembler allowing non-zero exit codes.
                returncode, stdout, stderr = execute_assembler(input_file_path, allow_failure=True)
                # Assert that the assembler indeed failed
                assert returncode != 0, "Test {}: Expected assembler to fail, but it succeeded.".format(test_name)

                # Check for expected error file (can be expected.error or expected.err)
                expected_err_file = test_directory / "expected.error"
                if not expected_err_file.exists():
                    expected_err_file = test_directory / "expected.err"

                if expected_err_file.exists():
                    expected_err = expected_err_file.read_text().strip()
                    actual_err = stderr.decode().strip()

                    # --- Portability: substitute tokens and normalize paths ---
                    test_as = str(input_file_path.resolve())
                    test_noext = str(input_file_path.resolve().with_suffix(''))
                    repo_root = str(THIS_DIRECTORY.parent.resolve())

                    def subst_tokens(s: str) -> str:
                        # Replace known tokens and normalize path separators for cross-platform consistency
                        return (s.replace("{TEST_AS}", test_as)
                                  .replace("{TEST_NOEXT}", test_noext)
                                  .replace("{REPO_ROOT}", repo_root)
                                  .replace("\\", "/"))

                    expected_norm = subst_tokens(expected_err)
                    actual_norm = actual_err.replace("\\", "/")

                    assert actual_norm == expected_norm, \
                        "Test {} failed: error output does not match expected.\n\nGot:\n{}\n\nExpected:\n{}".format(
                            test_name, actual_norm, expected_norm)
                else:
                    # If an error was expected but no expected error file exists, that's an issue.
                    raise FileNotFoundError(
                        "Test {}: Expected assembler to fail, but no 'expected.error' or 'expected.err' file found."
                        .format(test_name))
            else:
                # If success is expected, run the assembler and assert success.
                returncode, stdout, stderr = execute_assembler(input_file_path, allow_failure=False)
                assert returncode == 0, "Test {}: Assembler failed unexpectedly with error:\n{}".format(
                    test_name, stderr.decode())

                # Determine if this test is within the 'pass2' subdirectory
                # Using .as_posix() for consistent path string comparison across OS
                is_pass2_test = str(test_directory).startswith(str(CASES / "pass2"))

                if is_pass2_test:
                    # For pass2 tests, compare all relevant output files
                    _compare_multi_extension_outputs(input_file_path, test_directory, test_name)
                else:
                    # For other tests, revert to previous logic: only compare .am file
                    expected_am_file = test_directory / "expected.am"
                    actual_am_file = input_file_path.with_suffix('.am')

                    if not actual_am_file.exists():
                        raise FileNotFoundError(
                            "Test {}: Actual output file {} does not exist.".format(test_name, actual_am_file))
                    if not expected_am_file.exists():
                        raise FileNotFoundError(
                            "Test {}: Expected output file {} does not exist.".format(test_name, expected_am_file))

                    actual_content = actual_am_file.read_text().strip()
                    expected_content = expected_am_file.read_text().strip()

                    assert actual_content == expected_content, \
                        "Test {}: Output file '{}' content does not match expected.\n" \
                        "--- Got ---\n{}\n\n--- Expected ---\n{}".format(
                            test_name, actual_am_file.name, actual_content, expected_content)
        finally:
            # Always clean up generated files after the test, regardless of pass/fail
            cleanup_test()

    return test


# Dynamically build a unittest.TestCase subclass with one method per discovered test
class DynamicAssemblerTests(unittest.TestCase):
    pass


# Discover test cases and attach them as methods
for idx, (test_name, test_directory, is_expected_failure) in enumerate(discover_test_cases()):
    test_func = make_test_function(test_name, test_directory, is_expected_failure)
    # Assign a unique and descriptive name to the test function
    test_func.__name__ = "test_{:09d}_{}".format(
        idx, test_name.replace('/', '_').replace(' ', '_').replace('-', '_') # Sanitize name for method
    )
    setattr(DynamicAssemblerTests, test_func.__name__, test_func)


if __name__ == "__main__":
    compile_assembler() # Compile the assembler before running tests
    unittest.main()     # Run all discovered tests
else:
    # This block is for guidance if the script is imported, not run directly
    print("This script is intended to be run directly, not imported as a module.")
    print("Please run it from the command line or an appropriate test runner.")
