To build from Linux:

Ninja is used to build.
Install depot_tools as per http://www.chromium.org/developers/how-tos/install-depot-tools

./lbshell/build/gyp_driver lb_shell linux
ninja -C out/lb_shell/Linux_[Debug|Devel|QA|Gold]

output goes to out/lb_shell/
