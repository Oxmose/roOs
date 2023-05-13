import sys

if __name__ == "__main__":
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "| UTK UNIT TEST FRAMEWORK                                                      |" + COLORS.ENDC)
    print(COLORS.OKBLUE + COLORS.BOLD + "#==============================================================================#"  + COLORS.ENDC)
    if len(sys.argv) != 5:
        print("Usage: {} target test_group_file test_list test_file_output".format(sys.argv[0]))
        exit(1)

