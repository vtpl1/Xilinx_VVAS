import glob
import os
import subprocess
import sys
import traceback

from loguru import logger


def fail_if(p: bool, msg: str):
    if p:
        print("error: {}".format(msg), file=sys.stderr)
        sys.exit(1)


def get_cwd() -> str:
    cwd = os.path.dirname(os.path.realpath(__file__))
    if True:
        cwd = os.getcwd()
    return cwd


BUILD_DIR = "build"
BUILD_DIR = os.path.join(get_cwd(), BUILD_DIR)
EXE_NAME = "deeperlook"


def do_clang_format_file(file_name: str):
    func_name = "do_clang_format_file"
    runarguments = ["clang-format", "-i", file_name]
    cwd = get_cwd()
    logger.info(f"{func_name} start {runarguments} in folder {cwd}")
    try:
        p = subprocess.Popen(runarguments, cwd=cwd)
        p.wait()
        fail_if(p.returncode != 0, f"{func_name} failed")
        logger.info(f"{func_name} end")
    except Exception:
        logger.error(traceback.format_exc())
        logger.error(f"{func_name} failed")


def do_clang_format():
    func_name = "do_clang_format"
    logger.info(f"{func_name} start")
    all_files = []
    all_files.extend(glob.glob(os.path.join("utils", "*.c*"), recursive=True))
    all_files.extend(glob.glob(os.path.join("utils", "*.h"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vms_live_event_sender/**", "*.c*"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vms_live_event_sender/**", "*.h"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vvas_vtpl_fire_smoke/**", "*.c*"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vvas_vtpl_fire_smoke/**", "*.h"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vvas_vtpl_people_fall/**", "*.c*"), recursive=True))
    all_files.extend(glob.glob(os.path.join("vvas_vtpl_people_fall/**", "*.h"), recursive=True))
    # all_files.extend(
    #     glob.glob(
    #         os.path.join("datamodels", os.path.join("src", "*.cpp")), recursive=True
    #     )
    # )
    # all_files.extend(
    #     glob.glob(
    #         os.path.join("datamodels", os.path.join("inc", "*.h")), recursive=True
    #     )
    # )

    for f in all_files:
        do_clang_format_file(f)
    logger.info(f"{func_name} end")


def do_setup_meson():
    func_name = "do_setup_meson"
    runarguments = ["meson", "setup", BUILD_DIR]
    cwd = get_cwd()
    logger.info(f"{func_name} start {runarguments} in folder {cwd}")
    try:
        p = subprocess.Popen(runarguments, cwd=cwd)
        p.wait()
        fail_if(p.returncode != 0, f"{func_name} failed")
        logger.info(f"{func_name} end")
    except Exception:
        logger.error(traceback.format_exc())
        logger.error(f"{func_name} failed")


def do_compile_meson():
    func_name = "do_compile_meson"
    runarguments = ["meson", "compile"]
    cwd = BUILD_DIR
    logger.info(f"{func_name} start {runarguments} in folder {cwd}")
    try:
        p = subprocess.Popen(runarguments, cwd=cwd)
        p.wait()
        fail_if(p.returncode != 0, f"{func_name} failed")
        logger.info(f"{func_name} end")
    except Exception:
        logger.error(traceback.format_exc())
        logger.error(f"{func_name} failed")
    pass


def do_run_exe():
    func_name = "do_run_exe"
    exe = ""
    exe_prefix = ""
    exe_postfix = ""
    if sys.platform.startswith("win32"):
        exe_postfix = ".exe"
    else:
        exe_prefix = "./"

    exe = exe_prefix + EXE_NAME + exe_postfix

    runarguments = [exe]
    cwd = BUILD_DIR
    logger.info(f"{func_name} start {runarguments} in folder {cwd}")
    try:
        p = subprocess.Popen(runarguments, cwd=cwd)
        p.wait()
        fail_if(p.returncode != 0, f"{func_name} failed")
        logger.info(f"{func_name} end")
    except Exception:
        logger.error(traceback.format_exc())
        logger.error(f"{func_name} failed")

def do_ldd():
    func_name = "do_ldd"
    runarguments = ["ldd", f"{BUILD_DIR}/vvas_vtpl_people_fall/libvvas_vtpl_people_fall.so"]
    cwd = BUILD_DIR
    logger.info(f"{func_name} start {runarguments} in folder {cwd}")
    try:
        p = subprocess.Popen(runarguments, cwd=cwd)
        p.wait()
        fail_if(p.returncode != 0, f"{func_name} failed")
        logger.info(f"{func_name} end")
    except Exception:
        logger.error(traceback.format_exc())
        logger.error(f"{func_name} failed")
    pass

# ldd build/vvas_vtpl_people_fall/libvvas_vtpl_people_fall.so

do_clang_format()
if not os.path.exists(BUILD_DIR):
    do_setup_meson()
do_compile_meson()
do_ldd()
# do_run_exe()
