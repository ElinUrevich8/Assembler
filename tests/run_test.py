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
        # Compile the assembler
        subprocess.run(
            ["make"],
            check=True,
            cwd=str(SOURCE_CODE_DIRECTORY),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as e:
        print("Compilation failed with error:\n" + e.stderr.decode())
        raise e


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
        if allow_failure:
            return e.returncode, e.stdout, e.stderr
        print("Assembler failed with error:\n" + e.stderr.decode())
        raise e


def cleanup_test():
    """Remove assembler output files between tests."""
    # Recursively iterate on TESTS directory and delete specific output files
    for file_path in TESTS.rglob("test*"):
        if file_path.is_file():
            # Check if the file has one of the expected assembler output extensions
            ext = file_path.suffix.lower()
            if ext in {".am", ".ob", ".ent", ".ext"}:
                try:
                    file_path.unlink() # Delete the file
                except Exception as e:
                    # Log the error but don't crash the test suite on cleanup issues
                    print("Failed to delete {}: {}".format(file_path, e))
                    # You might want to raise here depending on desired strictness: raise e


def _compare_multi_extension_outputs(input_file_path, test_directory, test_name):
    """
    Compares produced files (*.ext, *.am, *.ent, *.ob) against their
    corresponding expected.<ext> files. This is used specifically for
    tests under `tests/cases/pass2`.
    """
    # Define the extensions to compare
    extensions_to_check = [".am", ".ob", ".ent", ".ext"]

    for ext in extensions_to_check:
        actual_path = input_file_path.with_suffix(ext)
        expected_path = test_directory / ("expected" + ext)

        # Check if the actual file was produced
        if actual_path.exists():
            # If produced, ensure there's an expected file to compare against
            if not expected_path.exists():
                raise FileNotFoundError(
                    "Test {}: Produced file {} exists, but missing corresponding expected file {}."
                    .format(test_name, actual_path.name, expected_path.name)
                )
            actual_text = actual_path.read_text().strip()
            expected_text = expected_path.read_text().strip()
            assert actual_text == expected_text, (
                "Test {}: Output file '{}' content does not match expected.\n"
                "--- Got ---\n{}\n\n--- Expected ---\n{}"
            ).format(test_name, actual_path.name, actual_text, expected_text)
        # If the actual file was NOT produced, but an expected file exists, it's an error.
        # This handles cases where a file *should* be produced but isn't.
        elif expected_path.exists():
            raise FileNotFoundError(
                "Test {}: Expected file {} exists, but actual output file {} was not produced."
                .format(test_name, expected_path.name, actual_path.name)
            )


def run_test(test_name, test_directory, is_expected_failure=False):
    """
    Runs a single assembler test case.
    Handles expected failures by checking stderr, and expected successes by comparing outputs.
    """
    input_file_path = test_directory / "test.as"
    
    # Ensure a clean slate before running each test
    cleanup_test()

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
                assert actual_err == expected_err, \
                    "Test {} failed: error output does not match expected.\n\nGot:\n{}\n\nExpected: {}".format(
                        test_name, actual_err, expected_err)
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


def discover_test_cases():
    """
    Discovers all test cases by looking for 'test.as' files within the CASES directory.
    Identifies expected failures based on the presence of 'expected.error' or 'expected.err'.
    """
    for test_dir in CASES.rglob("*"): # Recursively search all subdirectories
        test_as = test_dir / "test.as"
        if test_as.exists(): # If 'test.as' exists, it's a test case directory
            # Detect if test is an expected-failure case
            is_expected_failure = (test_dir / "expected.error").exists() or \
                                  (test_dir / "expected.err").exists()
            test_name = str(test_dir.relative_to(CASES)) # Get a relative path for the test name
            yield test_name, test_dir, is_expected_failure


def make_test_function(test_name, test_directory, is_expected_failure):
    """
    Helper to create a dynamic test method for unittest.TestCase.
    """
    def test(self):
        run_test(test_name, test_directory, is_expected_failure)
    return test


class DynamicAssemblerTests(unittest.TestCase):
    """
    A test suite class to which test methods will be dynamically added.
    """
    pass


# Dynamically create and add test functions to DynamicAssemblerTests
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