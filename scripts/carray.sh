#!/usr/bin/env bash

function usage()
{
	cat <<EOF >&2
Usage:  $0 [options]

Options:
     -h                   Display help or usage
     -i <input_config>    Input config file
     -l <variable_list>   List of variables in the array (Optional)
EOF
	exit 1;
}

# Command line options
CONFIG_FILE=""
VAR_LIST=""

while getopts "hi:l:" o; do
	case "${o}" in
	h)
		usage
		;;
	i)
		CONFIG_FILE=${OPTARG}
		;;
	l)
		VAR_LIST=${OPTARG}
		;;
	*)
		usage
		;;
	esac
done
shift $((OPTIND-1))

if [ -z "${CONFIG_FILE}" ]; then
	echo "Must specify input config file"
	usage
fi

if [ ! -f "${CONFIG_FILE}" ]; then
	echo "The input config file should be a present"
	usage
fi

TYPE_HEADER=$(awk '{ if ($1 == "HEADER:") { printf $2; exit 0; } }' "${CONFIG_FILE}")
if [ -z "${TYPE_HEADER}" ]; then
	echo "Must specify HEADER: in input config file"
	usage
fi

TYPE_NAME=$(awk '{ if ($1 == "TYPE:") { printf $2; for (i=3; i<=NF; i++) printf " %s", $i; exit 0; } }' "${CONFIG_FILE}")
if [ -z "${TYPE_NAME}" ]; then
	echo "Must specify TYPE: in input config file"
	usage
fi

ARRAY_NAME=$(awk '{ if ($1 == "NAME:") { printf $2; exit 0; } }' "${CONFIG_FILE}")
if [ -z "${ARRAY_NAME}" ]; then
	echo "Must specify NAME: in input config file"
	usage
fi

MEMBER_NAME=$(awk '{ if ($1 == "MEMBER-NAME:") { printf $2; exit 0; } }' "${CONFIG_FILE}")
MEMBER_TYPE=$(awk '{ if ($1 == "MEMBER-TYPE:") { printf $2; for (i=3; i<=NF; i++) printf " %s", $i; exit 0; } }' "${CONFIG_FILE}")
if [ -n "${MEMBER_NAME}" ] && [ -z "${MEMBER_TYPE}" ]; then
	echo "Must specify MEMBER-TYPE: when using MEMBER-NAME:"
	usage
fi

printf "// Generated with $(basename $0) from $(basename ${CONFIG_FILE})\n"
printf "#include <%s>\n\n" "${TYPE_HEADER}"

for VAR in ${VAR_LIST}; do
	printf "extern %s %s;\n" "${TYPE_NAME}" "${VAR}"
done
printf "\n"

printf "%s *const %s[] = {\n" "${MEMBER_TYPE:-${TYPE_NAME}}" "${ARRAY_NAME}"
for VAR in ${VAR_LIST}; do
	printf "\t&%s,\n" "${VAR}${MEMBER_NAME:+.}${MEMBER_NAME}"
done
	printf "\tNULL\n"
printf "};\n"
