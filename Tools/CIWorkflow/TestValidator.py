import json
import sys
import os
import re

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
    "RCODE"
]

TARGET_LIST = [
    "x86_64",
    "x86_i386"
]

def UpdateTestFile(filename, testGroup, testName):
    with open(filename, "r+") as fileDesc:
        fileContent   = fileDesc.readlines()
        startPosition = -1
        linesToRemove = []
        newFile       = []
        nameFound     = False

        # Find the lines to remove and the line where to start adding flags
        pattern = re.compile("TEST_.*?_ENABLED")
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
                if startPosition == -1:
                    startPosition = i
                linesToRemove.append(i)

            if startPosition == -1:
                newFile.append(fileContent[i])

        # If no start position was found, search for the constants section
        pattern = re.compile(" \* TESTING ENABLE FLAGS")
        for i in range(0, len(fileContent)):
            result = pattern.search(fileContent[i])

            if result != None:
                startPosition = i + 2

        # If still no start position found, return error
        if startPosition == -1:
            print("Error: cannot find the position to edit the tests list")
            exit(-1)

        # Add flags based on groups
        for testFlag in testGroup:
            newFile.append("#define TEST_{}_ENABLED 1\n".format(testFlag))

        # Add the rest of the file
        for i in range(startPosition, len(fileContent)):
            if i not in linesToRemove:
                newFile.append(fileContent[i])

        # Save file
        fileDesc.seek(0)
        fileDesc.truncate(0)
        for line in newFile:
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

            retValue = os.system("make clean > /dev/null")
            if retValue != 0:
                error += 1
                continue

            retValue = os.system("make target={} TESTS=TRUE > /dev/null 2>&1".format(target))
            if retValue != 0:
                error += 1
                continue


    print(COLORS.OKBLUE + COLORS.BOLD + "\n\n#==============================================================================#" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| FINAL REPORT                                                                 |" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Total:  {:<68} |".format(total)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Sucess: {:<68} |".format(success)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| Errors: {:<68} |".format(error)  + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)

    exit(error)