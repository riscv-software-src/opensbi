#!/bin/bash

function usage()
{
	echo "Usage:"
	echo " $0 [options]"
	echo "Options:"
	echo "     -h                   Display help or usage"
	echo "     -i <input_file_path> Input binary file path"
	echo "     -a <c_align>         Output C array alignment"
	echo "     -p <c_prefix>        Output C array name prefix"
	exit 1;
}

# Command line options
INPUT_PATH=""
OUTPUT_C_ALIGN=""
OUTPUT_C_PREFIX=""

while getopts "hi:a:p:o:" o; do
	case "${o}" in
	h)
		usage
		;;
	i)
		INPUT_PATH=${OPTARG}
		;;
	a)
		OUTPUT_C_ALIGN=${OPTARG}
		;;
	p)
		OUTPUT_C_PREFIX=${OPTARG}
		;;
	*)
		usage
		;;
	esac
done
shift $((OPTIND-1))

if [ -z "${INPUT_PATH}" ]; then
	echo "Must specify input file path"
	usage
fi

if [ ! -f "${INPUT_PATH}" ]; then
	echo "The input path should be a file"
	usage
fi

if [ -z "${OUTPUT_C_ALIGN}" ]; then
	echo "Must provide output C array alignment"
	usage
fi

if [ -z "${OUTPUT_C_PREFIX}" ]; then
	echo "Must provide output C array name prefix"
	usage
fi

printf "const char __attribute__((aligned(%s))) %s_start[] = {\n" "${OUTPUT_C_ALIGN}" "${OUTPUT_C_PREFIX}"

od -v -t x1 -An ${INPUT_PATH} | awk '{for (i=1; i<=NF; i++) printf " 0x%s,", $i; printf "\n"; }'

printf "};\n"

printf "const unsigned long %s_size = sizeof(%s_start);\n" "${OUTPUT_C_PREFIX}" "${OUTPUT_C_PREFIX}"
