import distutils
import numpy

distutils.core.setup(
    name="holocam",
    version="1.0.0",
    description="Python wrapper for the Holo virtual camera for windows 11",
    author="Simon Doksrød",
    author_email="simondok@stud.ntnu.no",
    ext_modules=[distutils.core.Extension("holocam", ["module.c"], include_dirs=[numpy.get_include()])],
    install_requires=["numpy"],
)
