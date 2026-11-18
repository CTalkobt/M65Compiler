# deps_table.sh: the packages the suite needs, per distribution family (#619).
# The one list: check_deps.sh (make check-deps, the Makefile's hints) prints
# it, src/test/test_distro_builds.sh installs it in a fresh container of each
# distribution (so the CI keeps it true), README.md repeats it.
# POSIX sh (sourced by sh on macOS and by busybox on Alpine): no arrays.
#
# deps_family FAMILY sets, for debian fedora rhel arch suse alpine macos:
#   DEP_NAME     the distributions of the family, for a heading
#   DEP_INSTALL  the command that installs packages (as a user would type it)
#   DEP_PRE      a command to run before it, or empty (RHEL: fuse3-devel is in CRB)
#   DEP_BASE     the C++17 compiler and GNU make            (required)
#   DEP_ZLIB     zlib's headers                             (required)
#   DEP_SQLITE   SQLite's headers: disk45 catalog           (optional)
#   DEP_FUSE     FUSE 3 and pkg-config: disk45 mount        (optional)
#   DEP_TEST     what `make test` uses beyond the build
# and returns 1 for a family it does not know.

DEP_FAMILIES="debian fedora rhel arch suse alpine macos"

deps_family() {
    DEP_PRE=
    case $1 in
    debian)
        DEP_NAME="Debian / Ubuntu"
        DEP_INSTALL="sudo apt-get install"
        DEP_BASE="build-essential"
        DEP_ZLIB="zlib1g-dev"
        DEP_SQLITE="libsqlite3-dev"
        DEP_FUSE="libfuse3-dev pkg-config"
        DEP_TEST="git python3 xxd bsdextrautils zip unzip" ;;
    fedora)
        DEP_NAME="Fedora"
        DEP_INSTALL="sudo dnf install"
        DEP_BASE="gcc-c++ make"
        DEP_ZLIB="zlib-ng-compat-devel"
        DEP_SQLITE="sqlite-devel"
        DEP_FUSE="fuse3-devel pkgconf-pkg-config"
        DEP_TEST="git python3 xxd util-linux diffutils zip unzip" ;;
    rhel)
        DEP_NAME="RHEL / Rocky / AlmaLinux"
        DEP_INSTALL="sudo dnf install"
        DEP_PRE="sudo dnf install dnf-plugins-core && sudo dnf config-manager --set-enabled crb"
        DEP_BASE="gcc-c++ make"
        DEP_ZLIB="zlib-devel"
        DEP_SQLITE="sqlite-devel"
        DEP_FUSE="fuse3-devel pkgconf-pkg-config"
        DEP_TEST="git python3 vim-common util-linux diffutils zip unzip" ;;
    arch)
        DEP_NAME="Arch Linux / Manjaro"
        DEP_INSTALL="sudo pacman -S --needed"
        DEP_BASE="base-devel"
        DEP_ZLIB="zlib"
        DEP_SQLITE="sqlite"
        DEP_FUSE="fuse3 pkgconf"
        DEP_TEST="git python tinyxxd util-linux diffutils zip unzip" ;;
    suse)
        DEP_NAME="openSUSE"
        DEP_INSTALL="sudo zypper install"
        DEP_BASE="gcc-c++ make"
        DEP_ZLIB="zlib-devel"
        DEP_SQLITE="sqlite3-devel"
        DEP_FUSE="fuse3-devel pkgconf"
        DEP_TEST="git python3 xxd util-linux diffutils findutils gawk zip unzip" ;;
    alpine)
        DEP_NAME="Alpine Linux"
        DEP_INSTALL="sudo apk add"
        DEP_BASE="build-base"
        DEP_ZLIB="zlib-dev"
        DEP_SQLITE="sqlite-dev"
        DEP_FUSE="fuse3-dev pkgconf"
        DEP_TEST="git python3 bash xxd hexdump zip unzip" ;;
    macos)
        # Apple clang is the Xcode command line tools' (xcode-select --install);
        # zlib and SQLite are in the macOS SDK; macOS's own make is 3.81 (enough
        # for the Makefile; the CI builds with Homebrew's gmake). No FUSE 3
        # there: disk45 is built without mount.
        DEP_NAME="macOS"
        DEP_INSTALL="brew install"
        DEP_BASE="make"
        DEP_ZLIB=""
        DEP_SQLITE=""
        DEP_FUSE=""
        DEP_TEST="" ;;
    *)
        DEP_NAME= DEP_INSTALL= DEP_BASE= DEP_ZLIB= DEP_SQLITE= DEP_FUSE= DEP_TEST=
        return 1 ;;
    esac
}
