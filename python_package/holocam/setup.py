import setuptools
import numpy

setuptools.setup(
    ext_modules=[setuptools.Extension("holocam._c", ["module.c"], include_dirs=[numpy.get_include()])],
    install_requires=["numpy"],
    data_files=[(".", ["holo_cam.dll"])],
)
