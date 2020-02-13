#!/bin/bash

function usage()
{
	echo "Usage:"
	echo " $0 [options]"
	echo "Options:"
	echo "     -h                       Display help or usage"
	echo "     -p <opensbi_source_path> OpenSBI source path"
	echo "     -o <build_output_path>   Build output path"
	echo "     -d                       Build and install documentation"
	echo "     -t                       Build only with no archive created"
	echo "     -j <num_threads>         Number of threads for Make (Default: 1)"
	echo "     -s <archive_suffix>      Archive name suffix (Default: unknown)"
	echo "     -x <riscv_xlen>          RISC-V XLEN (Default: 64)"
	exit 1;
}

# Command line options
BUILD_NUM_THREADS=1
BUILD_OUTPUT_PATH="$(pwd)/build"
BUILD_OPENSBI_SOURCE_PATH="$(pwd)"
BUILD_DOCS="no"
BUILD_ONLY="no"
BUILD_ARCHIVE_SUFFIX="unknown"
BUILD_RISCV_XLEN=64

while getopts "hdtj:o:p:s:x:" o; do
	case "${o}" in
	h)
		usage
		;;
	d)
		BUILD_DOCS="yes"
		;;
	t)
		BUILD_ONLY="yes"
		;;
	j)
		BUILD_NUM_THREADS=${OPTARG}
		;;
	o)
		BUILD_OUTPUT_PATH=${OPTARG}
		;;
	p)
		BUILD_OPENSBI_SOURCE_PATH=${OPTARG}
		;;
	s)
		BUILD_ARCHIVE_SUFFIX=${OPTARG}
		;;
	x)
		BUILD_RISCV_XLEN=${OPTARG}
		;;
	*)
		usage
		;;
	esac
done
shift $((OPTIND-1))

if [ -z "${BUILD_OPENSBI_SOURCE_PATH}" ]; then
	echo "Must specify OpenSBI source path"
	usage
fi

if [ ! -d "${BUILD_OPENSBI_SOURCE_PATH}" ]; then
	echo "OpenSBI source path does not exist"
	usage
fi

if [ -z "${BUILD_ARCHIVE_SUFFIX}" ]; then
	echo "Archive suffice cannot be empty"
	usage
fi

# Get version of OpenSBI
BUILD_VERSION_MAJOR=$(grep "define OPENSBI_VERSION_MAJOR" "${BUILD_OPENSBI_SOURCE_PATH}/include/sbi/sbi_version.h" | sed 's/.*MAJOR.*\([0-9][0-9]*\)/\1/')
BUILD_VERSION_MINOR=$(grep "define OPENSBI_VERSION_MINOR" "${BUILD_OPENSBI_SOURCE_PATH}/include/sbi/sbi_version.h" | sed 's/.*MINOR.*\([0-9][0-9]*\)/\1/')

# Setup archive name
BUILD_ARCHIVE_NAME="opensbi-${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}-rv${BUILD_RISCV_XLEN}-${BUILD_ARCHIVE_SUFFIX}"

# Setup platform list
case "${BUILD_RISCV_XLEN}" in
32)
	# Setup 32-bit platform list
	BUILD_PLATFORM_SUBDIR=("qemu/virt")
	;;
64)
	# Setup 64-bit platform list
	BUILD_PLATFORM_SUBDIR=("qemu/virt")
	BUILD_PLATFORM_SUBDIR+=("sifive/fu540")
	BUILD_PLATFORM_SUBDIR+=("kendryte/k210")
	BUILD_PLATFORM_SUBDIR+=("ariane-fpga")
	BUILD_PLATFORM_SUBDIR+=("andes/ae350")
	BUILD_PLATFORM_SUBDIR+=("thead/c910")
	BUILD_PLATFORM_SUBDIR+=("spike")
	;;
*)
	echo "Invalid RISC-V XLEN"
	usage
	;;
esac

# Ensure output directory is present
mkdir -p "${BUILD_OUTPUT_PATH}"

# Do a clean build first
make distclean

# Build and install generic library
echo "Build and install generic library XLEN=${BUILD_RISCV_XLEN}"
echo ""
make -C "${BUILD_OPENSBI_SOURCE_PATH}" O="${BUILD_OUTPUT_PATH}" I="${BUILD_OUTPUT_PATH}/${BUILD_ARCHIVE_NAME}" PLATFORM_RISCV_XLEN="${BUILD_RISCV_XLEN}" install_libsbi install_libsbiutils -j "${BUILD_NUM_THREADS}"
echo ""

# Build and install relevant platforms
for INDEX in $(seq 0 1 "$(expr ${#BUILD_PLATFORM_SUBDIR[*]} - 1)")
do
	echo "Build and install PLATFORM=${BUILD_PLATFORM_SUBDIR[${INDEX}]} XLEN=${BUILD_RISCV_XLEN}"
	echo ""
	make -C "${BUILD_OPENSBI_SOURCE_PATH}" O="${BUILD_OUTPUT_PATH}" I="${BUILD_OUTPUT_PATH}/${BUILD_ARCHIVE_NAME}" PLATFORM="${BUILD_PLATFORM_SUBDIR[${INDEX}]}" PLATFORM_RISCV_XLEN="${BUILD_RISCV_XLEN}" install_libplatsbi install_firmwares -j "${BUILD_NUM_THREADS}"
	echo ""
done

# Build and install docs
if [ "${BUILD_DOCS}" == "yes" ]; then
	echo "Build and install docs"
	echo ""
	make -C "${BUILD_OPENSBI_SOURCE_PATH}" O="${BUILD_OUTPUT_PATH}" I="${BUILD_OUTPUT_PATH}/${BUILD_ARCHIVE_NAME}" install_docs
	echo ""
fi

# Create archive file
if [ "${BUILD_ONLY}" == "no" ]; then
	echo "Create archive ${BUILD_ARCHIVE_NAME}.tar.xz"
	echo ""
	tar  -C "${BUILD_OUTPUT_PATH}" -cJvf "${BUILD_OUTPUT_PATH}/${BUILD_ARCHIVE_NAME}.tar.xz" "${BUILD_ARCHIVE_NAME}"
	echo ""
fi
