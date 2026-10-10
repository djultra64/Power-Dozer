"""Resolve the legacy reference layout or an independent SDK layout."""
from pathlib import Path

def sdk_paths(*,sdk_root=None,tetrisphere=None):
    if (sdk_root is None)==(tetrisphere is None):raise ValueError('Choose exactly one SDK root or Tetrisphere reference')
    if sdk_root is not None:
        base=Path(sdk_root).resolve()
        return {'deps':base/'deps','tools':base/'toolchain/n64recomp-build','rt64_build':base/'rt64-build','analysis_venv':base/'analysis-venv'}
    base=Path(tetrisphere).resolve()
    return {'deps':base/'.local/deps','tools':base/'.local/m1/toolchain/n64recomp-build','rt64_build':base/'.local/m1/build-linux/rt64','analysis_venv':base/'.local/m1/analysis-venv'}


def sdk_cmake_options(*,sdk_root=None,tetrisphere=None):
    paths=sdk_paths(sdk_root=sdk_root,tetrisphere=tetrisphere)
    return ['-DBC_SDK_ROOT='+str(Path(sdk_root).resolve() if sdk_root else ''),
            '-DTETRISPHERE_ROOT='+str(Path(tetrisphere).resolve() if tetrisphere else ''),
            '-DBC_RT64_BUILD_ROOT='+str(paths['rt64_build'])]
