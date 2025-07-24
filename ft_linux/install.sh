#!/bin/bash
set -e

# Critical packages that should NOT be replaced (due to system stability risks)
declare -a critical_packages=(
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
    ncurses
    readline
)

# Package name mapping for Debian
declare -A pkg_map=(
    [Acl]="acl"
    [Attr]="attr"
    [Autoconf]="autoconf"
    [Automake]="automake"
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

# Install via apt if version matches
install_with_apt() {
    local pkg_name=$1
    local required_version=$2
    local deb_pkg=${pkg_map[$pkg_name]}
    
    echo "Checking $pkg_name ($required_version)"
    
    # Skip critical packages
    if [[ " ${critical_packages[@]} " =~ " ${deb_pkg} " ]]; then
        echo "  [SKIP] Critical package - not replacing system version"
        return
    fi
    
    # Check if package exists in repos
    if ! apt-cache show "$deb_pkg" &> /dev/null; then
        echo "  [ERROR] Package not in repositories: $deb_pkg"
        return
    fi
    
    # Check version availability
    local installable_version
    installable_version=$(apt-cache policy "$deb_pkg" | grep -oP '(?<=Candidate: ).+')
    if [[ "$installable_version" == "$required_version" ]]; then
        echo "  [APT] Installing $deb_pkg=$required_version"
        apt-get install -y "$deb_pkg=$required_version"
    else
        echo "  [WARN] Wrong version available (need $required_version, found $installable_version)"
        echo "  [APT] Installing latest version instead"
        apt-get install -y "$deb_pkg"
    fi
}

# Install from source (generic method)
install_from_source() {
    local pkg_name=$1
    local version=$2
    local base_url=$3
    
    echo "  [SOURCE] Compiling $pkg_name-$version"
    local build_dir="/tmp/${pkg_name}-build"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    wget "${base_url}/${pkg_name}-${version}.tar.gz"
    tar -xzf "${pkg_name}-${version}.tar.gz"
    cd "${pkg_name}-${version}"
    
    ./configure --prefix=/usr/local
    make -j$(nproc)
    make install
    
    cd ~
    rm -rf "$build_dir"
}

# Main installation process
main() {
    # Check root privileges
    [[ $EUID -eq 0 ]] || {
        echo "This script must be run as root. Use 'sudo' or run as root user."
        exit 1
    }
    
    # Update package list
    apt-get update
    
    # Install build dependencies
    apt-get install -y build-essential wget
    
    # Process packages
    while IFS= read -r line; do
        if [[ "$line" =~ ([^[:space:]]+)[[:space:]]+\(([0-9a-zA-Z.-]+)\) ]]; then
            pkg_name="${BASH_REMATCH[1]}"
            version="${BASH_REMATCH[2]}"
            
            case "$pkg_name" in
                GCC|Glibc|Binutils|GRUB)
                    echo "[SKIP] Critical toolchain component: $pkg_name - not modifying"
                    ;;
                *)
                    if [[ -v "pkg_map[$pkg_name]" ]]; then
                        install_with_apt "$pkg_name" "$version"
                    else
                        echo "[WARN] No installation method for: $pkg_name"
                    fi
                    ;;
            esac
        fi
    done < <(grep -oE '[^•]+\([^)]+\)' <<< "$(tail -n +2 "$0")")
    
    # Special cases
    echo "Processing special installations:"
    install_from_source "Acl" "2.2.52" "http://download.savannah.gnu.org/releases/acl"
    install_from_source "Attr" "2.4.47" "http://download.savannah.gnu.org/releases/attr"
    install_from_source "Automake" "1.15" "http://ftp.gnu.org/gnu/automake"
    
    echo "Installation attempt completed. Some packages may require manual verification."
}

main
exit 0

# Package list follows (for script parsing):
# Acl (2.2.52)
# Attr (2.4.47)
# Autoconf (2.69)
# Automake (1.15)
# Bash (4.3.30)
# Bc (1.06.95)
# Binutils (2.25.1)
# Bison (3.0.4)
# Bzip2 (1.0.6)
# Check (0.10.0)
# Coreutils (8.24)
# DejaGNU (1.5.3)
# Diffutils (3.3)
# Eudev (3.1.2)
# E2fsprogs (1.42.13)
# Expat (2.1.0)
# Expect (5.45)
# File (5.24)
# Findutils (4.4.2)
# Flex (2.5.39)
# Gawk (4.1.3)
# GCC (5.2.0)
# GDBM (1.11)
# Gettext (0.19.5.1)
# Glibc (2.22)
# GMP (6.0.0a)
# Gperf (3.0.4)
# Grep (2.21)
# Groff (1.22.3)
# GRUB (2.02 beta2)
# Gzip (1.6)
# Iana-Etc (2.30)
# Inetutils (1.9.4)
# Intltool (0.51.0)
# IPRoute2 (4.2.0)
# Kbd (2.0.3)
# Kmod (21)
# Less (458)
# Libcap (2.24)
# Libpipeline (1.4.1)
# Libtool (2.4.6)
# M4 (1.4.17)
# Make (4.1)
# Man-DB (2.7.2)
# Man-pages (4.02)
# MPC (1.0.3)
# MPFR (3.1.3)
# Ncurses (6.0)
# Patch (2.7.5)
# Perl (5.22.0)
# Pkg-config (0.28)
# Procps (3.3.11)
# Psmisc (22.21)
# Readline (6.3)
# Sed (4.2.2)
# Shadow (4.2.1)
# Sysklogd (1.5.1)
# Sysvinit (2.88dsf)
# Tar (1.28)
# Tcl (8.6.4)
# Texinfo (6.0)
# Time Zone Data (2015f)
# Udev-lfs Tarball (udev-lfs-20140408)
# Util-linux (2.27)
# Vim (7.4)
# XML::Parser (2.44)
# Xz Utils (5.2.1)
# Zlib (1.2.8)