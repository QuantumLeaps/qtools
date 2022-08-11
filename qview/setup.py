#!/usr/bin/env python
'''
Setup script.
To build qview install:
[sudo] python setup.py sdist bdist_wheel
'''

from setuptools import setup

setup(
    name="qview",
    version="7.0.0",
    author="Quantum Leaps",
    author_email="info@state-machine.com",
    description="QView visualization and monitoring",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://www.state-machine.com/qtools/qview.html",
    license="GPL/commercial",
    platforms="any",
    py_modules=["qview"],
    entry_points={"console_scripts": ["qview = qview:main"]},
    classifiers=["Development Status :: 5 - Production/Stable",
                 "Intended Audience :: Developers",
                 "Topic :: Software Development :: Embedded Systems",
                 "Topic :: System :: Monitoring",
                 "Topic :: System :: Logging",
                 "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)",
                 "License :: Other/Proprietary License",
                 "Operating System :: Microsoft :: Windows",
                 "Operating System :: POSIX :: Linux",
                 "Operating System :: MacOS :: MacOS X",
                 "Programming Language :: Python",
                 "Programming Language :: Python :: 3"],
)
