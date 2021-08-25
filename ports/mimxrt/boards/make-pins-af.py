import re
import argparse
from collections import defaultdict

regexes = [
    r"IOMUXC_(?P<pin>GPIO_SD_B\d_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_AD_B\d_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_EMC_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_EMC_\w\d_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_SNVS_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_LPSR_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>WAKEUP_DIG)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_DISP_\w\d_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_B\d_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_AD_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
    r"IOMUXC_(?P<pin>GPIO_SD_\d\d)_(?P<function>\w+) (?P<muxRegister>\w+), (?P<muxMode>\w+), (?P<inputRegister>\w+), (?P<inputDaisy>\w+), (?P<configRegister>\w+)",
]


def main(input_path, output_path):
    pins = defaultdict(dict)

    with open(input_path, "r") as iomuxc_file:
        input_str = iomuxc_file.read()

        for regex in regexes:
            matches = re.finditer(regex, input_str, re.MULTILINE)

            for match in matches:
                pins[match.group("pin")][
                    int((match.groupdict()["muxMode"].strip("U")), 16)
                ] = match.group("function")

    header = "Pad,ALT0,ALT1,ALT2,ALT3,ALT4,ALT5,ALT6,ALT7,ALT8,ALT9,ALT10,ADC,ACMP,Default\n"

    with open(output_path, "w") as output_file:
        output_file.write(header)

        for pin_name, pin_dict in pins.items():
            line = pin_name + ", "
            for i in range(11):
                line += pin_dict.get(i, "") + ","
            # Append dummy entries for analog functions
            line += ","  # ACMP
            line += ","  # ADC

            line += "\n"

            output_file.write(line)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="make-pins-af.py",
        usage="%(prog)s [options] [command]",
        description="Generate board specific pin af file",
    )
    parser.add_argument(
        "-i",
        "--iomuxc",
        dest="iomuxc_file",
        help="Path to the fsl_iomuxc.h file for device",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output_file",
        help="Specifies the output file path",
    )
    #
    args = parser.parse_args(
        [
            "-i",
            "/home/philipp/Projects/micropython/micropython/lib/nxp_driver/sdk/devices/MIMXRT1176/drivers/fsl_iomuxc.h",
            "-o",
            "1176_iomux.csv",
        ]
    )
    #
    main(args.iomuxc_file, args.output_file)
