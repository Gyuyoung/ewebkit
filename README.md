# ewebkit
svn checkout -r HEAD --depth=empty https://svn.webkit.org/repository/webkit/trunk/ ewebkit
svn up ewebkit/CMakeLists.txt ewebkit/Source ewebkit/ChangeLog
sed -i '/-Werror/d' ewebkit/Source/cmake/WebKitHelpers.cmake
mkdir build
cd build
export LDFLAGS="$LDFLAGS -lpthread"
cmake ../$pkgname \
    -DPORT=Efl \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SKIP_RPATH=ON \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DLIB_INSTALL_DIR=/usr/lib \
    -DLIBEXEC_INSTALL_DIR=/usr/lib/$pkgname \
    -DENABLE_GEOLOCATION=OFF \
    -DENABLE_SPEECH_SYNTHESIS=OFF \
    -DENABLE_MEDIA_STREAM=OFF
make -j$(grep -F processor /proc/cpuinfo -c)
make DESTDIR="../dist" install
