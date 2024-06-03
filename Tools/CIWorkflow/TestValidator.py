import json
import sys
import os
import re
import subprocess
import time

class COLORS:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

TYPE_STR = [
    "BYTE",
    "UBYTE",
    "HALF",
    "UHALF",
    "WORD",
    "UWORD",
    "DWORD",
    "UDWORD",
    "FLOAT",
    "DOUBLE",
    "RCODE",
    "POINTER"
]

TARGET_LIST = [
    "x86_64",
    "x86_i386"
]

def Validate(jsonTestsuite):
    print(COLORS.OKCYAN + COLORS.BOLD + "#--------------------------------------------------#" + COLORS.ENDC)
    print(COLORS.OKCYAN + COLORS.BOLD + "| UTK Test Suite                                   |" + COLORS.ENDC)
    print(COLORS.OKCYAN + COLORS.BOLD + "#--------------------------------------------------#" + COLORS.ENDC)
    print(COLORS.OKCYAN +"| Version: {:40s}|".format(jsonTestsuite["version"]) + COLORS.ENDC)
    print(COLORS.OKCYAN +"| Testname: {:39s}|".format(jsonTestsuite["name"]) + COLORS.ENDC)
    print(COLORS.OKCYAN +"#--------------------------------------------------#" + COLORS.ENDC)
    print(COLORS.OKCYAN +"| N# of tests    | N# of success  | N# of failures |" + COLORS.ENDC)
    print(COLORS.OKCYAN +"|--------------------------------------------------#" + COLORS.ENDC)
    print(COLORS.OKCYAN +"| {:14d} | {:14d} | {:14d} |".format(jsonTestsuite["number_of_tests"], jsonTestsuite["success"], jsonTestsuite["failures"]) + COLORS.ENDC)
    print(COLORS.OKCYAN +"#--------------------------------------------------#" + COLORS.ENDC)

    outComeStr = ""

    testIdDict = {}

    for testId, testContent in jsonTestsuite["test_suite"].items():
        testIdDict[testId] = 1
        if testContent["status"] == 0:
            outComeStr = COLORS.FAIL + COLORS.BOLD + "FAIL" + COLORS.ENDC
            print("===> Test {}".format(testId))
            print("    > Outcome: {} | Expected: 0x{:X} -- Result: 0x{:X} | Type: {}".format(outComeStr, testContent["expected"], testContent["result"], TYPE_STR[testContent["type"]]))
        #else:
        #    outComeStr = COLORS.OKGREEN + COLORS.BOLD + "PASS" + COLORS.ENDC


    print()
    if jsonTestsuite["failures"] == 0:
        print(COLORS.OKGREEN + COLORS.BOLD + "==== " + jsonTestsuite["name"] + " RESULT: PASS ====" + COLORS.ENDC)
    else:
        print(COLORS.FAIL + COLORS.BOLD + "==== " + jsonTestsuite["name"] + " RESULT: FAIL ====" + COLORS.ENDC)

    print("\n#--------------------------------------------------------------------------------#\n")

    # Validate the test
    return jsonTestsuite["failures"]


def DictRaiseDuplicate(ordPairs):
    """Reject duplicate keys."""
    d = {}
    for k, v in ordPairs:
        if k in d:
           raise ValueError("Duplicate test ID: %r" % (k,))
        else:
           d[k] = v
    return d

def ParseInputFile(filename):
    isTestsuiteContent = False
    with open(filename, 'r', errors='ignore') as fileDesc:
        fileContent = fileDesc.readlines()
        jsonBody = ""
        for line in fileContent:
            line = line.strip("\n")
            if line == "#-------- TESTING SECTION START --------#":
                isTestsuiteContent = True
            elif line == "#-------- TESTING SECTION END --------#":
                isTestsuiteContent = False
            else:
                if isTestsuiteContent:
                    jsonBody += line + "\n"

        if len(jsonBody) == 0:
            return ""

        jsonObject = json.loads(jsonBody, object_pairs_hook=DictRaiseDuplicate)

        return jsonObject

def UpdateTestFile(filename, testGroup, testName):
    with open(filename, "r+") as fileDesc:
        fileContent   = fileDesc.readlines()
        startPosition = -1
        nameFound     = False

        # Find the lines to remove and the line where to start adding flags
        pattern = re.compile("TEST_(.*?)_ENABLED")
        namePattern = re.compile("TEST_FRAMEWORK_TEST_NAME")
        for i in range(0, len(fileContent)):
            if not nameFound:
                result = namePattern.search(fileContent[i])
                if result != None:
                    nameFound = True
                    fileContent[i] = "#define TEST_FRAMEWORK_TEST_NAME \"{}\"\n".format(testName)
                    continue

            result = pattern.search(fileContent[i])

            if result != None:
                # Disable the flag
                newLine = "#define {}".format(result.group(0))
                fileContent[i] = newLine + " " * (50 - len(newLine)) + "0\n"
                if startPosition == -1:
                    startPosition = i

        # If no start position was found, search for the constants section
        if startPosition == -1:
            pattern = re.compile(" \* TESTING ENABLE FLAGS")
            for i in range(0, len(fileContent)):
                result = pattern.search(fileContent[i])

                if result != None:
                    startPosition = i + 2

        # If still no start position found, return error
        if startPosition == -1:
            print("Error: cannot find the position to edit the tests list")
            exit(-1)

        # Add or enable flags based on groups
        for testFlag in testGroup:
            found = False
            pattern = re.compile("TEST_{}_ENABLED".format(testFlag))
            newLine = "#define TEST_{}_ENABLED".format(testFlag)
            newLine = newLine + " " * (50 - len(newLine)) + "1\n"
            for i in range(startPosition, len(fileContent)):
                result = pattern.search(fileContent[i])
                if result != None:
                    fileContent[i] = newLine
                    found = True
                    break
            if not found:
                fileContent.insert(startPosition + 1, newLine)

        # Save file
        fileDesc.seek(0)
        fileDesc.truncate(0)
        for line in fileContent:
            fileDesc.write(line)

        print("> Updated Test List file")


if __name__ == "__main__":
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| UTK UNIT TEST FRAMEWORK                                                      |" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)
    if len(sys.argv) != 5:
        print("Usage: {} target test_group_file test_list test_file_output".format(sys.argv[0]))
        exit(1)

    target             = sys.argv[1]
    testGroupsFileName = sys.argv[2]
    testListFileName   = sys.argv[3]
    testOutputFileName = sys.argv[4]

    error   = 0
    success = 0
    total   = 0

    if target not in TARGET_LIST:
        print("Error: Unknown target {}, only {} are supported".format(target, TARGET_LIST))

    with open(testGroupsFileName) as groupFile:
        jsonObject = json.loads(groupFile.read())

        for group in jsonObject:
            print(COLORS.OKBLUE + "\n#==============================================================================#" + COLORS.ENDC)
            print(COLORS.OKBLUE + " > Executing Group {}".format(group["name"])  + COLORS.ENDC)
            print(COLORS.OKBLUE + " > " + str(group["group"])  + COLORS.ENDC)
            print(COLORS.OKBLUE + " > Target {}".format(target) + COLORS.ENDC)
            print(COLORS.OKBLUE + "#==============================================================================#\n"  + COLORS.ENDC)

            total += 1

            # Update test file
            UpdateTestFile(testListFileName, group["group"], group["name"])

            retValue = os.system("make clean")
            if retValue != 0:
                error += 1
                continue

            retValue = os.system("make target={} TESTS=TRUE".format(target, group["name"], target))
            if retValue != 0:
                error += 1
                continue

            start = time.time()
            with open(testOutputFileName, "w") as outputFile:
                p = subprocess.Popen(["make", "target={}".format(target), "qemu-test-mode"], stdout = outputFile)
                try:
                    p.wait(30)
                except subprocess.TimeoutExpired:
                    p.kill()

                outputFile.close()
            print("Tests took {:.2f}ms".format(1000 * (time.time() - start)))
            os.sync()

            jsonTestsuite = ParseInputFile(testOutputFileName)
            if len(jsonTestsuite) != 0:
                retValue = Validate(jsonTestsuite)
                if retValue == 0:
                    success += 1
                else:
                    error += 1
            else:
                print("Error: testing result were not printed.")
                error += 1

    print(COLORS.OKBLUE + COLORS.BOLD + "\n\n#==============================================================================#" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| FINAL REPORT                                                                 |" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Total:  {:<68} |".format(total)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Sucess: {:<68} |".format(success)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Errors: {:<68} |".format(error)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)

    exit(error)