./build_install_vvas.sh TARGET=PCIE PLATFORM=V70 ENABLE_XRM=0 ENABLE_XRM_PLG=0
mkdir -p /workspaces/Xilinx_VVAS/vtpl_lib/
sudo cp /opt/xilinx/vvas/lib/libvvas_vtpl_people_fall.so /workspaces/Xilinx_VVAS/vtpl_lib/
sudo cp /opt/xilinx/vvas/lib/libvvas_vtpl_fire_smoke.so /workspaces/Xilinx_VVAS/vtpl_lib/
cp /workspaces/Xilinx_VVAS/vvas-accel-sw-libs/build/subprojects/nng-1.5.2/libnng.so /workspaces/Xilinx_VVAS/vtpl_lib/