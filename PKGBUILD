# Script generated with Bloom
pkgdesc="ROS - The pluginlib package provides tools for writing and dynamically loading plugins using the ROS build infrastructure. To work, these tools require plugin providers to register their plugins in the package.xml of their package."
url='http://www.ros.org/wiki/pluginlib'

pkgname='ros-lunar-pluginlib'
pkgver='1.11.3_1'
pkgrel=1
arch=('any')
license=('BSD'
)

makedepends=('boost'
'ros-lunar-catkin>=0.5.68'
'ros-lunar-class-loader'
'ros-lunar-cmake-modules'
'ros-lunar-rosconsole'
'ros-lunar-roslib>=1.11.1'
'tinyxml2'
)

depends=('boost'
'ros-lunar-class-loader'
'ros-lunar-rosconsole'
'ros-lunar-roslib>=1.11.1'
'tinyxml2'
)

conflicts=()
replaces=()

_dir=pluginlib
source=()
md5sums=()

prepare() {
    cp -R $startdir/pluginlib $srcdir/pluginlib
}

build() {
  # Use ROS environment variables
  source /usr/share/ros-build-tools/clear-ros-env.sh
  [ -f /opt/ros/lunar/setup.bash ] && source /opt/ros/lunar/setup.bash

  # Create build directory
  [ -d ${srcdir}/build ] || mkdir ${srcdir}/build
  cd ${srcdir}/build

  # Fix Python2/Python3 conflicts
  /usr/share/ros-build-tools/fix-python-scripts.sh -v 2 ${srcdir}/${_dir}

  # Build project
  cmake ${srcdir}/${_dir} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCATKIN_BUILD_BINARY_PACKAGE=ON \
        -DCMAKE_INSTALL_PREFIX=/opt/ros/lunar \
        -DPYTHON_EXECUTABLE=/usr/bin/python2 \
        -DPYTHON_INCLUDE_DIR=/usr/include/python2.7 \
        -DPYTHON_LIBRARY=/usr/lib/libpython2.7.so \
        -DPYTHON_BASENAME=-python2.7 \
        -DSETUPTOOLS_DEB_LAYOUT=OFF
  make
}

package() {
  cd "${srcdir}/build"
  make DESTDIR="${pkgdir}/" install
}

