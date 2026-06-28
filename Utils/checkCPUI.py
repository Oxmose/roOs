if __name__ == "__main__":
    hostValues = []
    qemuValues = []
    rawQemuValues = []

    with open("./checkhost", 'r') as hostFile:
        baseValues = hostFile.readlines()
        for i in range(len(baseValues)):
            if baseValues[i][-1] == "\n":
                baseValues[i] = baseValues[i][0:-1]
            if len(baseValues[i]) == 0:
                continue

            hostValues.append(baseValues[i])

    with open("./checkqemu", 'r') as qemuFile:
        baseValues = qemuFile.readlines()
        for i in range(len(baseValues)):
            if baseValues[i][-1] == "\n":
                baseValues[i] = baseValues[i][0:-1]
            if len(baseValues[i]) == 0:
                continue
            splitVal = baseValues[i].split(" ")
            qemuValues.append([splitVal[0], splitVal[2]])

    # Check the qemu values
    foundAndValid = []
    notFoundAndValid = []
    foundAndInvalid = []
    notFoundAndInvalid = []

    for value in qemuValues:
        if value[1] == "true":
            rawQemuValues.append(value[0])
            if value[0] not in hostValues:
                foundAndInvalid.append(value[0])
            else:
                foundAndValid.append(value[0])
        elif value[1] == "false":
            if value[0] not in hostValues:
                notFoundAndValid.append(value[0])
            else:
                notFoundAndInvalid.append(value[0])
        else:
            print(value[1])
            print("ERROR")
            exit(1)

    for value in hostValues:
        if value not in rawQemuValues:
            notFoundAndInvalid.append(value)

    print("==== Found and Valid")
    print(foundAndValid)
    print("==== Not Found and Valid")
    print(notFoundAndValid)
    print("==== Found and Invalid")
    print(foundAndInvalid)
    print("==== Not Found and Invalid")
    print(notFoundAndInvalid)