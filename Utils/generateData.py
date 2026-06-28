
import xml.etree.ElementTree as ET
if __name__ == "__main__":
    fileOutDefine = open("OutputDefine.txt", 'w')
    fileOutStruct = open("OutputStruct.txt", 'w')
    fileCode = open("OutputCode.txt", 'w')
    tree = ET.parse('./leaf01.xml')
    root = tree.getroot()
    for child in root:
        for register in child:
            regName = register.tag.upper()
            print(regName)

            for field in register:
                if not "bit" in field.tag:
                    continue
                bitValue = int(field.tag.replace("bit", ""))
                length = int(field.attrib['len'])
                identifier = field.attrib['id'].upper()
                description = field.attrib['desc']

                if length != 1:
                    print("Element {} is not a bit, skipping...".format(identifier))
                    continue
                fileOutDefine.write("/** @brief {}. */\n".format(description))
                fileOutDefine.write("#define {}_{} (1U << {})\n".format(regName, identifier, bitValue))
                fileOutStruct.write("\t/** @brief {}. */\n".format(description))
                fileOutStruct.write("\tbool {} : 1;\n".format(identifier.lower()))
                fileCode.write("\tpCpuInf.{} = ((regs[{}_REG] & {}_{}) == {}_{});\n".format(identifier.lower(), regName, regName, identifier, regName, identifier))
