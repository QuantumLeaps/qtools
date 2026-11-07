#!/usr/bin/env python
'''
Setup script.
To build the qutest package (qutest, qimpact, qutestify):
[sudo] python setup.py sdist bdist_wheel
'''

from setuptools import setup

setup(
    name="qutest",
    version="8.1.6",
    author="Quantum Leaps",
    author_email="info@state-machine.com",
    description="QUTest Python scripting support",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://www.state-machine.com/qtools/qutest.html",
    license="LicenseRef-QL-commercial",
    platforms="any",
    py_modules=["qutest", "qimpact", "qutestify"],
    entry_points={"console_scripts": ["qutest = qutest:main",
                                      "qimpact = qimpact:main",
                                      "qutestify = qutestify:main"]},
    classifiers=["Development Status :: 5 - Production/Stable",
                 "Intended Audience :: Developers",
                 "Topic :: Software Development :: Embedded Systems",
                 "Topic :: Software Development :: Testing",
                 "Topic :: Software Development :: Testing :: Unit",
                 "Operating System :: Microsoft :: Windows",
                 "Operating System :: POSIX :: Linux",
                 "Operating System :: MacOS :: MacOS X",
                 "Programming Language :: Python",
                 "Programming Language :: Python :: 3",
                 "Programming Language :: C",
                 "Programming Language :: C++"],
)
