"""
 /******************************************************************************
 * @file TraceConverter.py
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 1.0
 *
 * @brief UTK Trace buffer converter.
 *
 * @details UTK Trace buffer converter. This scripts take a trace buffer dump
 * binary file and converts it to a TraceCompass readable file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
"""

import argparse

from pathlib import Path

def GetArguments():
    parser = argparse.ArgumentParser()

    # Add arguments
    parser.add_argument("--dict", type=str, required=True)
    parser.add_argument("--input", type=str, required=True)

    # Get arguments
    args = parser.parse_args()

    return args

if __name__ == "__main__":

    print("#===============================================#")
    print("| UTK Trace Converter                           |")
    print("#===============================================#")

    # Get arguments
    args = GetArguments()

    # Transform the output file name
    outputFilename = Path(args.name).stem + "_converted.json"
    print("==> Input:  {}")
    print("==> Output: {}".format(outputFilename))

    # Get the events and their properties

    # Convert the trace buffer