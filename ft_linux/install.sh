#!/bin/bash
set -e

# Function to wait for dpkg lock release
wait_for_dpkg_lock() {
    echo "Checking for dpkg lock..."
    local max_attempts=30
    local wait_seconds=10
    local attempt=0

    while fuser /var/lib/dpkg/lock-frontend >/dev/null 2>&1 || \
          fuser /var/lib/dpkg/lock >/dev/null 2>&1; do
        attempt=$((attempt + 1))
        if [ $attempt -gt $max_attempts ]; then
            echo "Error: Could not acquire dpkg lock after $((max_attempts * wait_seconds)) seconds"
            echo "Check for running package managers:"
            ps aux | grep -E 'apt|dpkg|aptd'
            exit 1
        fi
        echo "Lock held by another process (attempt $attempt/$max_attempts), waiting $wait_seconds seconds..."
        sleep $wait_seconds
    done
    echo "Lock clear - proceeding"
}

# Package name mapping for Debian
declare -A pkg_map=(
    [Acl]="acl"
    [Attr]="attr"
    [Autoconf]="autoconf"
    [Automake]="automake"
    [Bash]="bash"
    [Bc]="bc"
    [Bison]="bison"
    [Bzip2]="bzip2"
    [Check]="check"
    [DejaGNU]="dejagnu"
    [Diffutils]="diffutils"
    [Eudev]="eudev"
    [E2fsprogs]="e2fsprogs"
    [Expat]="libexpat1-dev"
    [Expect]="expect"
    [File]="file"
    [Findutils]="findutils"
    [Flex]="flex"
    [Gawk]="gawk"
    [GDBM]="gdbm"
    [Gettext]="gettext"
    [GMP]="libgmp-dev"
    [Gperf]="gperf"
    [Grep]="grep"
    [Groff]="groff"
    [Gzip]="gzip"
    [Iana-Etc]="iana-etc"
    [Inetutils]="inetutils"
    [Intltool]="intltool"
    [IPRoute2]="iproute2"
    [Kbd]="kbd"
    [Less]="less"
    [Libcap]="libcap-dev"
    [Libpipeline]="libpipeline-dev"
    [Libtool]="libtool"
    [M4]="m4"
    [Make]="make"
    [Man-DB]="man-db"
    [Man-pages]="manpages"
    [MPC]="libmpc-dev"
    [MPFR]="libmpfr-dev"
    [Patch]="patch"
    [Pkg-config]="pkg-config"
    [Procps]="procps"
    [Psmisc]="psmisc"
    [Readline]="libreadline-dev"
    [Sed]="sed"
    [Sysklogd]="sysklogd"
    [Sysvinit]="sysvinit-core"
    [Tcl]="tcl-dev"
    [Texinfo]="texinfo"
    [Time Zone Data]="tzdata"
    [Util-linux]="util-linux"
    [Vim]="vim"
    [XML::Parser]="libxml-parser-perl"
    [Xz Utils]="xz-utils"
    [Zlib]="zlib1g-dev"
)

# Critical packages that should NOT be replaced
critical_packages=(
    glibc
    gcc
    binutils
    coreutils
    bash
    sed
    shadow
    util-linux
    perl
    tar
    gzip
    bzip2
    xz-utils
    zlib
    libc-bin
    linux-base
    sysvinit
    kmod
    grub
)

# Install via apt if version matches
install_with_apt() {
    local pkg_name=$1
    local required_version=$2
    local deb_pkg=${pkg_map[$pkg_name]}
    
    # Skip if no mapping found
    if [ -z "$deb_pkg" ]; then
        echo "  [SKIP] No package mapping for: $pkg_name"
        return
    fi
    
    # Skip critical packages
    for crit_pkg in "${critical_packages[@]}"; do
        if [ "$deb_pkg" = "$crit_pkg" ]; then
            echo "  [SKIP] Critical package - not replacing system version: $deb_pkg"
            return
        fi
    done
    
    # Check package availability
    if ! apt-cache show "$deb_pkg" &>/dev/null; then
        echo "  [ERROR] Package not available: $deb_pkg"
        return
    fi
    
    # Check version
    local installable_version
    installable_version=$(apt-cache policy "$deb_pkg" | awk '/Candidate:/ {print $2}')
    
    if [[ "$installable_version" == "$required_version"* ]]; then
        echo "  [APT] Installing $deb_pkg=$installable_version"
        wait_for_dpkg_lock
        DEBIAN_FRONTEND=noninteractive apt-get install -y --allow-downgrades "$deb_pkg=$installable_version"
    else
        echo "  [WARN] Version mismatch:"
        echo "    Required: $required_version"
        echo "    Available: $installable_version"
        echo "  [APT] Installing available version: $deb_pkg"
        wait_for_dpkg_lock
        DEBIAN_FRONTEND=noninteractive apt-get install -y "$deb_pkg"
    fi
}

# Install from source with case-insensitive directory handling
install_from_source() {
    local pkg=$1
    local version=$2
    local url=$3
    
    echo "  [SOURCE] Building $pkg-$version"
    local build_dir="/tmp/${pkg}-build"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    echo "    Downloading from $url"
    local tarball="${pkg}-${version}.tar.gz"
    if ! wget -q "${url}/${tarball}"; then
        echo "    [ERROR] Download failed: ${tarball}"
        return
    fi
    
    echo "    Extracting source"
    tar -xzf "${tarball}"
    
    # Case-insensitive directory search
    local extracted_dir=$(find . -maxdepth 1 -type d -iname "${pkg}-*" | head -1)
    if [ -z "$extracted_dir" ]; then
        echo "    [ERROR] Could not find extraction directory for ${tarball}"
        return
    fi
    
    echo "    Entering directory: ${extracted_dir}"
    cd "${extracted_dir}"
    
    echo "    Configuring..."
    ./configure --prefix=/usr/local
    
    echo "    Compiling with $(nproc) cores..."
    make -j$(nproc)
    
    echo "    Installing..."
    make install
    
    echo "    Cleaning up..."
    cd /tmp
    rm -rf "$build_dir"
}

# Main installation process
main() {
    # Check root
    if [ "$EUID" -ne 0 ]; then
        echo "Please run as root"
        exit 1
    fi
    
    # Create package list with corrected case
    declare -a packages=(
        "acl 2.2.52 http://download.savannah.gnu.org/releases/acl"
        "attr 2.4.47 http://download.savannah.gnu.org/releases/attr"
        "autoconf 2.69 http://ftp.gnu.org/gnu/autoconf"
        "automake 1.15 http://ftp.gnu.org/gnu/automake"
        "bash 4.3.30 http://ftp.gnu.org/gnu/bash"
        "bc 1.06.95 http://ftp.gnu.org/gnu/bc"
        "bison 3.0.4 http://ftp.gnu.org/gnu/bison"
        "bzip2 1.0.6 http://www.bzip.org"
        "check 0.10.0 https://github.com/libcheck/check/releases/download/0.10.0"
        "diffutils 3.3 http://ftp.gnu.org/gnu/diffutils"
        "expect 5.45 http://sourceforge.net/projects/expect/files/Expect"
        "file 5.24 https://astron.com/pub/file"
        "findutils 4.4.2 http://ftp.gnu.org/gnu/findutils"
        "flex 2.5.39 https://github.com/westes/flex/releases/download/v2.5.39"
        "gawk 4.1.3 http://ftp.gnu.org/gnu/gawk"
        "gdbm 1.11 http://ftp.gnu.org/gnu/gdbm"
        "gettext 0.19.5.1 http://ftp.gnu.org/gnu/gettext"
        "gperf 3.0.4 http://ftp.gnu.org/gnu/gperf"
        "groff 1.22.3 http://ftp.gnu.org/gnu/groff"
        "intltool 0.51.0 https://launchpad.net/intltool/trunk/0.51.0/+download"
        "libpipeline 1.4.1 http://download.savannah.gnu.org/releases/libpipeline"
        "libtool 2.4.6 http://ftpmirror.gnu.org/libtool"
        "m4 1.4.17 http://ftp.gnu.org/gnu/m4"
        "man-db 2.7.2 http://download.savannah.gnu.org/releases/man-db"
        "mpc 1.0.3 http://www.multiprecision.org/downloads"
        "tcl 8.6.4 https://prdownloads.sourceforge.net/tcl"
    )
    
    # Update package lists with lock handling
    wait_for_dpkg_lock
    echo "Updating package lists..."
    DEBIAN_FRONTEND=noninteractive apt-get update -y
    
    # Install build dependencies
    wait_for_dpkg_lock
    echo "Installing build dependencies..."
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential \
        wget \
        ca-certificates \
        gcc \
        make \
        perl \
        bison \
        flex \
        libssl-dev \
        libncurses-dev
    
    # Process packages
    for pkg_info in "${packages[@]}"; do
        read -r pkg version url <<< "$pkg_info"
        echo "Processing $pkg ($version)"
        
        # Use lowercase for all source installations
        install_from_source "$pkg" "$version" "$url"
    done
    
    echo "Installation completed. Note: Some critical packages were skipped."
    echo "Please verify versions with: dpkg -l | grep -E '${critical_packages[*]}'"
}

main