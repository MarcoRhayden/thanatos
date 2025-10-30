sudo pacman -Syu base-devel git curl zip unzip tar cmake ninja
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd ..
./vcpkg/vcpkg install
